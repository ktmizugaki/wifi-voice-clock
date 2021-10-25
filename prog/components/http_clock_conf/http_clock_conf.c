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
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <esp_http_server.h>

#include <http_html_cmn.h>
#include <json_str.h>

#include "http_clock_conf.h"
#include "clock_conf.h"

#define TAG "clock_conf"
#define CLOCK_CONF_URI  "/clock_conf"

static MAKE_EMBEDDED_HANDLER(http_clock_conf_html, "text/html")

static esp_err_t http_get_conf_handler(httpd_req_t *req)
{
    json_str_t *json;
    clock_conf_t conf;
    esp_err_t err;

    clock_conf_get(&conf);

    json = new_json_str(72);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);
    json_str_add_string(json, "TZ", conf.TZ);
    json_str_add_integer(json, "sync_weeks", conf.sync_weeks);
    json_str_add_integer(json, "sync_time", conf.sync_time);
    json_str_end_object(json);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static void post_conf_params_handler(char *key, size_t key_len, char *value, size_t value_len, void *user_data)
{
    clock_conf_t *params = user_data;

    if (value == NULL) return;
    if (HTTP_CMN_KEYCMP(key, key_len, "TZ")) {
        if (value_len+1 <= sizeof(params->TZ)) {
            strcpy(params->TZ, value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "sync_weeks")) {
        int weeks = atoi(value);
        if (weeks >= 0 && weeks <= 0x7f) {
            params->sync_weeks = weeks;
        } else {
            ESP_LOGI(TAG, "Invalid weeks: %s", value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "sync_time")) {
        int time = atoi(value);
        if (time >= 0 && time < 86400) {
            params->sync_time = time;
        } else {
            ESP_LOGI(TAG, "Invalid time: %s", value);
        }
        return;
    }
}

static esp_err_t http_post_conf_handler(httpd_req_t *req)
{
    json_str_t *json;
    clock_conf_t params;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    params.sync_weeks = 0xff;
    params.sync_time = -1;
    err = http_cmn_handle_form_data(req, post_conf_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return http_cmn_send_error_json(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.TZ[0] == '\0' || params.sync_weeks == 0xff || params.sync_time == -1) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }

    err = clock_conf_set(&params);
    if (err != ESP_OK) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Failed to set");
        goto end;
    }

    json_str_add_integer(json, "status", 1);
    json_str_add_string(json, "message", "Updated clock conf");

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_get_time_handler(httpd_req_t *req)
{
    json_str_t *json;
    time_t t;
    esp_err_t err;

    time(&t);
    json = new_json_str(40);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);
    json_str_add_number(json, "time", t);
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static void post_time_params_handler(char *key, size_t key_len, char *value, size_t value_len, void *user_data)
{
    struct timeval *epoch = user_data;

    if (value == NULL) return;
    if (HTTP_CMN_KEYCMP(key, key_len, "time")) {
        epoch->tv_sec = atof(value);
        return;
    }
}

static esp_err_t http_post_time_handler(httpd_req_t *req)
{
    json_str_t *json;
    struct timeval epoch;
    struct timezone utc = {0,0};
    esp_err_t err;

    epoch.tv_sec = 0;
    epoch.tv_usec = 0;
    err = http_cmn_handle_form_data(req, post_time_params_handler, &epoch);
    if (err != HTTP_CMN_OK) {
        return http_cmn_send_error_json(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (epoch.tv_sec == 0) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }

    settimeofday(&epoch, &utc);

    json_str_add_integer(json, "status", 1);
    json_str_add_string(json, "message", "Updated time");

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_clock_conf_handler(httpd_req_t *req)
{
    const char *path = req->uri+sizeof(CLOCK_CONF_URI)-1;
    size_t path_len = strlen(path);
#define test_path(target_method, target_path) \
    (req->method == target_method && \
        path_len == sizeof(target_path)-1 && strcmp(path, target_path) == 0)

    if (test_path(HTTP_GET, "")) {
        return EMBEDDED_HANDLER_NAME(http_clock_conf_html)(req);
    }
    if (test_path(HTTP_GET, "/conf")) {
        return http_get_conf_handler(req);
    }
    if (test_path(HTTP_POST, "/conf")) {
        return http_post_conf_handler(req);
    }
    if (test_path(HTTP_GET, "/time")) {
        return http_get_time_handler(req);
    }
    if (test_path(HTTP_POST, "/time")) {
        return http_post_time_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
    return ESP_FAIL;
}

esp_err_t http_clock_conf_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.uri = CLOCK_CONF_URI "*";
    http_uri.handler = http_clock_conf_handler;
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

esp_err_t http_clock_conf_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, CLOCK_CONF_URI "*", HTTP_GET);
    httpd_unregister_uri_handler(handle, CLOCK_CONF_URI "*", HTTP_POST);
    httpd_unregister_uri_handler(handle, CLOCK_CONF_URI "*", HTTP_DELETE);
    return ESP_OK;
}
