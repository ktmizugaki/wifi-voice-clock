/* Copyright 2021 Kawashima Teruaki
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <string.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <esp_http_server.h>

#include <http_html_cmn.h>
#include <json_str.h>

#include "spiffs_upd.h"
#include "http_firmware.h"

#define TAG "firmware"
#define FIRMWARE_URI  "/firmware"
#define OCTET_STREAM_TYPE   "application/octet-stream"

#define BUF_SIZE    1024
#define MIN_IMAGE_SIZE  0x8000

static void ota_restart_task(void *arg)
{
    (void)arg;
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Restart due to OTA");
    esp_restart();
}

typedef struct {
    const esp_partition_t *partition;
    size_t image_len;
    int64_t elapsed;
} ota_work_t;

static MAKE_EMBEDDED_HANDLER(http_firmware_html, "text/html")

static esp_err_t send_bad_request_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, json);
    return ESP_FAIL;
}

static bool is_octet_stream(httpd_req_t *req)
{
    char content_type[sizeof(OCTET_STREAM_TYPE)+4];
    esp_err_t err;

    err = httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Content-Type: %s", content_type);
    }
    return (err == ESP_OK && strcmp(content_type, OCTET_STREAM_TYPE) == 0);
}

static esp_err_t http_get_version_handler(httpd_req_t *req)
{
    json_str_t *json;
    const esp_app_desc_t *desc;
    esp_err_t err;

    json = new_json_str(256);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);
    desc = esp_ota_get_app_description();
    json_str_add_string(json, "app_version", desc->version);
    if (json_str_begin_string(json, "datetime", 24) == JSON_STR_OK) {
      json_str_append_string(json, desc->date);
      json_str_append_string(json, "  ");
      json_str_append_string(json, desc->time);
      json_str_end_string(json);
    }
    json_str_add_string(json, "idf_version", desc->idf_ver);
    json_str_end_object(json);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_get_partinfo_handler(httpd_req_t *req)
{
    json_str_t *json;
    const esp_partition_t *partition;
    uint32_t appsize, spiffssize;
    esp_err_t err;

    partition = esp_ota_get_next_update_partition(NULL);
    if (partition != NULL) {
        appsize = partition->size;
    } else {
        appsize = 0;
    }
    partition = spiffs_upd_find_partition(NULL);
    if (partition != NULL) {
        spiffssize = partition->size;
    } else {
        spiffssize = 0;
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);
    json_str_add_integer(json, "appsize", appsize);
    json_str_add_integer(json, "spiffssize", spiffssize);
    json_str_end_object(json);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t update_firmware(httpd_req_t *req, ota_work_t *work)
{
    esp_ota_handle_t handle;
    char *buf;
    int recv_size;
    int remaining_size;
    int64_t start_time, feedwdt_time;
    esp_err_t err;

    buf = malloc(BUF_SIZE);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Start OTA update");
    handle = 0;
    err = esp_ota_begin(work->partition, work->image_len, &handle);
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    start_time = feedwdt_time = esp_timer_get_time();
    vTaskDelay(1);

    remaining_size = work->image_len;
    while (remaining_size > 0) {
        recv_size = MIN(remaining_size, BUF_SIZE);

        recv_size = httpd_req_recv(req, buf, recv_size);
        if (recv_size <= 0) {
            err = ESP_ERR_TIMEOUT;
            goto fail;
        }

        err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_write(handle, buf, recv_size));
        if (err != ESP_OK) {
            goto fail;
        }

        remaining_size -= recv_size;
        if (esp_timer_get_time() - feedwdt_time >= (CONFIG_TASK_WDT_TIMEOUT_S*1000000)/2) {
            feedwdt_time = esp_timer_get_time();
            vTaskDelay(1);
        }
    }

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_end(handle));
    handle = 0;
    if (err != ESP_OK) {
        goto fail;
    }

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_set_boot_partition(work->partition));
    if (err != ESP_OK) {
        goto fail;
    }

    work->elapsed = esp_timer_get_time() - start_time;
    ESP_LOGI(TAG, "Finished writing firmware.");
    free(buf);
    return ESP_OK;

fail:
    if (handle != 0) {
        esp_ota_end(handle);
    }
    ESP_LOGI(TAG, "OTA update failed");
    free(buf);
    return err;
}

static esp_err_t http_post_update_handler(httpd_req_t *req)
{
    ota_work_t work;
    char json[96];
    esp_err_t err;

    work.partition = esp_ota_get_next_update_partition(NULL);
    if (work.partition == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    work.image_len = req->content_len;

    ESP_LOGI(TAG, "target partition: ota_%d, address=%#x, size=%#x, name=%.16s",
        work.partition->subtype-ESP_PARTITION_SUBTYPE_APP_OTA_MIN,
        work.partition->address, work.partition->size, work.partition->label);

    if (work.image_len == 0) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Content-Length is required\"}");
    }
    if (work.image_len < MIN_IMAGE_SIZE) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Image is too small\"}");
    }
    if (work.image_len > work.partition->size) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Image is too large\"}");
    }

    if (!is_octet_stream(req)) {
        if (httpd_req_get_hdr_value_len(req, "Content-Type") == 0) {
            return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Content-Type is required\"}");
        } else {
            return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Unsupported Content-Type\"}");
        }
    }

    err = update_firmware(req, &work);

    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
            http_cmn_send_error_json(req, HTTP_CMN_ERR_SOCK_TIMEOUT);
        } else if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            http_cmn_send_error_json(req, HTTP_CMN_ERR_INVALID_REQ);
        } else {
            http_cmn_send_error_json(req, err);
        }
        return ESP_FAIL;
    }

    snprintf(json, sizeof(json), "{\"status\":-1,\"message\":\"Firmware updated in %d.%06d sec\\nRebooting...\"}",
        (int)(work.elapsed/1000000), (int)(work.elapsed%1000000));
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json);

    xTaskCreate(ota_restart_task, "ota_restart_task", 8*1024, NULL, 10, NULL);
    return err;
}

static esp_err_t update_spiffs(httpd_req_t *req, ota_work_t *work)
{
    esp_ota_handle_t handle;
    char *buf;
    int recv_size;
    int remaining_size;
    int64_t start_time, feedwdt_time;
    esp_err_t err;

    buf = malloc(BUF_SIZE);
    if (buf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Start SPIFFS update");
    handle = 0;
    err = spiffs_upd_begin(work->partition, work->image_len, &handle);
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    start_time = feedwdt_time = esp_timer_get_time();
    vTaskDelay(1);

    remaining_size = work->image_len;
    while (remaining_size > 0) {
        recv_size = MIN(remaining_size, BUF_SIZE);

        recv_size = httpd_req_recv(req, buf, recv_size);
        if (recv_size <= 0) {
            err = ESP_ERR_TIMEOUT;
            goto fail;
        }

        err = ESP_ERROR_CHECK_WITHOUT_ABORT(spiffs_upd_write(handle, buf, recv_size));
        if (err != ESP_OK) {
            goto fail;
        }

        remaining_size -= recv_size;
        if (esp_timer_get_time() - feedwdt_time >= (CONFIG_TASK_WDT_TIMEOUT_S*1000000)/2) {
            feedwdt_time = esp_timer_get_time();
            vTaskDelay(1);
        }
    }

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(spiffs_upd_end(handle));
    handle = 0;
    if (err != ESP_OK) {
        goto fail;
    }

    work->elapsed = esp_timer_get_time() - start_time;
    ESP_LOGI(TAG, "Finished writing spiffs.");
    free(buf);
    return ESP_OK;

fail:
    if (handle != 0) {
        spiffs_upd_end(handle);
    }
    ESP_LOGI(TAG, "spiffs update failed");
    free(buf);
    return err;
}

static esp_err_t http_post_spiffs_handler(httpd_req_t *req)
{
    ota_work_t work;
    char json[96];
    esp_err_t err;

    work.partition = spiffs_upd_find_partition(NULL);
    if (work.partition == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    if (work.partition->encrypted) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    work.image_len = req->content_len;

    ESP_LOGI(TAG, "target partition: %.16s", work.partition->label);

    if (work.image_len == 0) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Content-Length is required\"}");
    }
    if (work.image_len < work.partition->size) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Image is too small\"}");
    }
    if (work.image_len > work.partition->size) {
        return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Image is too large\"}");
    }

    if (!is_octet_stream(req)) {
        if (httpd_req_get_hdr_value_len(req, "Content-Type") == 0) {
            return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Content-Type is required\"}");
        } else {
            return send_bad_request_json(req, "{\"status\":-1,\"message\":\"Unsupported Content-Type\"}");
        }
    }

    err = update_spiffs(req, &work);

    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
            http_cmn_send_error_json(req, HTTP_CMN_ERR_SOCK_TIMEOUT);
        } else if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            http_cmn_send_error_json(req, HTTP_CMN_ERR_INVALID_REQ);
        } else {
            http_cmn_send_error_json(req, err);
        }
        return ESP_FAIL;
    }

    snprintf(json, sizeof(json), "{\"status\":-1,\"message\":\"SPIFFS updated in %d.%06d sec\\nRebooting...\"}",
        (int)(work.elapsed/1000000), (int)(work.elapsed%1000000));
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json);

    xTaskCreate(ota_restart_task, "ota_restart_task", 8*1024, NULL, 10, NULL);
    return err;
}

static esp_err_t http_firmware_handler(httpd_req_t *req)
{
    const char *path = req->uri+sizeof(FIRMWARE_URI)-1;
    size_t path_len = strlen(path);
#define test_path(target_method, target_path) \
    (req->method == target_method && \
        path_len == sizeof(target_path)-1 && strcmp(path, target_path) == 0)

    if (test_path(HTTP_GET, "")) {
        return EMBEDDED_HANDLER_NAME(http_firmware_html)(req);
    }
    if (test_path(HTTP_GET, "/version")) {
        return http_get_version_handler(req);
    }
    if (test_path(HTTP_GET, "/partinfo")) {
        return http_get_partinfo_handler(req);
    }
    if (test_path(HTTP_POST, "/update")) {
        return http_post_update_handler(req);
    }
    if (test_path(HTTP_POST, "/spiffs")) {
        return http_post_spiffs_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
    return ESP_FAIL;
}

esp_err_t http_firmware_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.uri = FIRMWARE_URI "*";
    http_uri.handler = http_firmware_handler;
    http_uri.user_ctx = NULL;

    http_uri.method = HTTP_GET;
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }
    http_uri.method = HTTP_POST;
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t http_firmware_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, FIRMWARE_URI "*", HTTP_GET);
    httpd_unregister_uri_handler(handle, FIRMWARE_URI "*", HTTP_POST);
    return ESP_OK;
}
