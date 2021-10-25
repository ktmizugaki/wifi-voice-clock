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

#include "http_firmware.h"

#define TAG "firmware"
#define FIRMWARE_URI  "/firmware"

static MAKE_EMBEDDED_HANDLER(http_firmware_html, "text/html")

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

static esp_err_t http_post_update_handler(httpd_req_t *req)
{
    const char *json;

    json = "{\"status\":0,\"message\":\"Not implemented yet\"}";

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    return httpd_resp_sendstr(req, json);
}

static esp_err_t http_post_spiffs_handler(httpd_req_t *req)
{
    const char *json;

    json = "{\"status\":0,\"message\":\"Not implemented yet\"}";

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    return httpd_resp_sendstr(req, json);
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
