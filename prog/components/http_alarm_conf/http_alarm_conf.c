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
#include <stdlib.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include <http_html_cmn.h>
#include <json_str.h>

#include "http_alarm_conf.h"
#include "alarm.h"

#define TAG "alarm_conf"
#define ALARM_CONF_URI  "/alarm_conf"

static MAKE_EMBEDDED_HANDLER(http_alarm_conf_html, "text/html")

enum alarm_param_filed {
    APF_INDEX = 1<<0,
    APF_ENABLED = 1<<1,
    APF_NAME = 1<<2,
    APF_WEEKS = 1<<3,
    APF_SECONDS = 1<<4,
    APF_ALARM_ID = 1<<5,
};

struct alarm_params {
    unsigned fields;
    int index;
    struct alarm alarm;
};

static esp_err_t alarm_conf_send_error(httpd_req_t *req, esp_err_t err)
{
    const char *status;
    const char *msg;
    if (err == HTTP_CMN_ERR_INVALID_REQ) {
        status = "400 Bad Request";
        msg = "{\"status\":-1,\"message\":\"Bad Request\"}";
    } else if (err == HTTP_CMN_ERR_SOCK_TIMEOUT) {
        status = "408 Request Timeout";
        msg = "{\"status\":-1,\"message\":\"Server closed this connection\"}";
    } else {
        status = "500 Internal Server Error";
        msg = "{\"status\":-1,\"message\":\"Server Error\"}";
    }
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, msg);
    return ESP_FAIL;
}

static esp_err_t http_get_alarms_handler(httpd_req_t *req)
{
    json_str_t *json;
    const struct alarm *alarms;
    int num_alarm;
    int i;
    esp_err_t err;

    alarm_init();
    alarm_get_alarms(&alarms, &num_alarm);

    json = new_json_str(24+68*num_alarm);
    if (json == NULL) {
        return alarm_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);

    json_str_begin_array(json, "alarms");
    for (i = 0; i < num_alarm; i++) {
        const struct alarm *palarm = &alarms[i];
        json_str_begin_object(json, NULL);
        json_str_add_boolean(json, "enabled", palarm->enabled);
        json_str_add_string(json, "name", palarm->name);
        json_str_add_integer(json, "weeks", palarm->weeks);
        json_str_add_integer(json, "seconds", palarm->seconds);
        json_str_add_integer(json, "alarm_id", palarm->alarm_id);
        json_str_end_object(json);
    }
    json_str_end_array(json);

    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static void post_alarms_params_handler(char *key, size_t key_len, char *value, size_t value_len, void *user_data)
{
    struct alarm_params *params = user_data;

    if (value == NULL) return;
    if (HTTP_CMN_KEYCMP(key, key_len, "index")) {
        params->fields |= APF_INDEX;
        params->index = atoi(value);
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "enabled")) {
        if (http_cmn_is_true_like(value)) {
            params->fields |= APF_ENABLED;
            params->alarm.enabled = true;
        } else if (http_cmn_is_false_like(value)) {
            params->fields |= APF_ENABLED;
            params->alarm.enabled = false;
        } else {
            ESP_LOGI(TAG, "invalid enabled: %s", value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "name")) {
        if (value_len+1 <= sizeof(params->alarm.name)) {
            params->fields |= APF_NAME;
            strcpy(params->alarm.name, value);
        } else {
            ESP_LOGI(TAG, "name is too long: %d", (int)value_len);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "weeks")) {
        int weeks = atoi(value);
        if (weeks >= 0 && weeks <= 0x7f) {
            params->fields |= APF_WEEKS;
            params->alarm.weeks = weeks;
        } else {
            ESP_LOGI(TAG, "Invalid weeks: %s", value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "seconds")) {
        int seconds = atoi(value);
        if (seconds >= 0 && seconds < 86400) {
            params->fields |= APF_SECONDS;
            params->alarm.seconds = seconds;
        } else {
            ESP_LOGI(TAG, "Invalid seconds: %s", value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "alarm_id")) {
        int alarm_id = atoi(value);
        if (alarm_id >= 0 && alarm_id < 1) {
            params->fields |= APF_ALARM_ID;
            params->alarm.alarm_id = alarm_id;
        } else {
            ESP_LOGI(TAG, "Invalid alarm_id: %s", value);
        }
        return;
    }
}

static esp_err_t http_post_alarms_handler(httpd_req_t *req)
{
    json_str_t *json;
    struct alarm_params params;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    params.index = -1;
    err = http_cmn_handle_form_data(req, post_alarms_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return alarm_conf_send_error(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return alarm_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if ((params.fields&0x3f) != 0x3f) {
        ESP_LOGD(TAG, "Missing params: fields: %#x", params.fields);
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }

    err = alarm_set_alarm(params.index, &params.alarm);
    if (err != ESP_OK) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Failed to save alarm");
        goto end;
    }

    json_str_add_integer(json, "status", 1);
    json_str_add_string(json, "message", "Updated alarm");

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_alarm_conf_handler(httpd_req_t *req)
{
    const char *path = req->uri+sizeof(ALARM_CONF_URI)-1;
    size_t path_len = strlen(path);
#define test_path(target_method, target_path) \
    (req->method == target_method && \
        path_len == sizeof(target_path)-1 && strcmp(path, target_path) == 0)

    if (test_path(HTTP_GET, "")) {
        return EMBEDDED_HANDLER_NAME(http_alarm_conf_html)(req);
    }
    if (test_path(HTTP_GET, "/alarms")) {
        return http_get_alarms_handler(req);
    }
    if (test_path(HTTP_POST, "/alarms")) {
        return http_post_alarms_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
    return ESP_FAIL;
}

esp_err_t http_alarm_conf_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.uri = ALARM_CONF_URI "*";
    http_uri.handler = http_alarm_conf_handler;
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

esp_err_t http_alarm_conf_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, ALARM_CONF_URI "*", HTTP_GET);
    httpd_unregister_uri_handler(handle, ALARM_CONF_URI "*", HTTP_POST);
    return ESP_OK;
}
