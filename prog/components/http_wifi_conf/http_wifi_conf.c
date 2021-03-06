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
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_wifi_types.h>

#include <simple_wifi.h>
#include <simple_wifi_event.h>
#include <http_html_cmn.h>
#include <json_str.h>

#include "http_wifi_conf.h"
#include "wifi_conf.h"
#include "internal.h"

#define TAG "wifi_conf"
#define WIFI_CONF_URI  "/wifi_conf"

static MAKE_EMBEDDED_HANDLER(http_wifi_conf_html, "text/html")

static esp_err_t http_get_scan_handler(httpd_req_t *req)
{
    json_str_t *json;
    int i, ap_num;
    const wifi_ap_record_t *ap;
    enum simple_wifi_scan_state state;
    esp_err_t err;

    state = simple_wifi_get_scan_result(&ap_num, (const void**)&ap);

    json = new_json_str(state == SIMPLE_WIFI_SCAN_DONE? 128: 32);
    if (json == NULL) {
        simple_wifi_release_scan_result(ap);
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }
    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", (int)state);
    if (state == SIMPLE_WIFI_SCAN_DONE) {
        json_str_begin_array(json, "records");
        for (i = 0; i < ap_num; i++) {
            char authmode[64];
            json_str_begin_object(json, NULL);
            json_str_add_string(json, "ssid", (const char*)ap[i].ssid);
            json_str_add_integer(json, "rssi", ap[i].rssi);
            if (ap[i].authmode >= WIFI_AUTH_WEP) {
                strcpy(authmode, simple_wifi_auth_mode_to_str(ap[i].authmode));
                strcat(authmode, "-");
                strcat(authmode, simple_wifi_cipher_type_to_str(ap[i].pairwise_cipher));
                json_str_add_string(json, "authmode", authmode);
            }
            json_str_end_object(json);
        }
        json_str_end_array(json);
    }
    json_str_end_object(json);
    simple_wifi_release_scan_result(ap);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_post_scan_handler(httpd_req_t *req)
{
    const char *json;
    esp_err_t err;

    if (simple_wifi_is_scan_result_available()) {
        return http_get_scan_handler(req);
    } else {
        err = simple_wifi_scan();
        if (err == ESP_OK) {
            json = "{\"status\":1}";
        } else {
            json = "{\"status\":0}";
        }
    }

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    return httpd_resp_sendstr(req, json);
}

static esp_err_t http_get_aps_handler(httpd_req_t *req)
{
    json_str_t *json;
    int i;
    esp_err_t err;

    wifi_conf_load();

    json = new_json_str(16+sizeof(struct wifi_conf)*wifi_conf_get_count());
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);

    json_str_begin_array(json, "aps");
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        const struct wifi_conf *conf = wifi_conf_get(i);
        if (conf == NULL) {
            continue;
        }
        json_str_begin_object(json, NULL);
        json_str_add_string(json, "ssid", conf->conf.ap.ssid);
        json_str_add_boolean(json, "use_static_ip", conf->conf.ap.use_static_ip);
        json_str_add_string(json, "ntp", conf->ntp);
        if (conf->conf.ap.use_static_ip) {
            json_str_add_string(json, "ip", conf->conf.ip);
            json_str_add_string(json, "gateway", conf->conf.gateway);
            json_str_add_string(json, "netmask", conf->conf.netmask);
        }
        json_str_end_object(json);
    }
    json_str_end_array(json);

    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static void post_aps_params_handler(char *key, size_t key_len, char *value, size_t value_len, void *user_data)
{
    struct wifi_conf *params = user_data;

    if (value == NULL) return;
    if (HTTP_CMN_KEYCMP(key, key_len, "ssid")) {
        if (value_len+1 <= sizeof(params->conf.ap.ssid)) {
            strcpy(params->conf.ap.ssid, value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "password")) {
        if (value_len+1 <= sizeof(params->conf.ap.password)) {
            strcpy(params->conf.ap.password, value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "use_static_ip")) {
        if (http_cmn_is_true_like(value)) {
            params->conf.ap.use_static_ip = 1;
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "ip")) {
        if (value_len+1 <= sizeof(params->conf.ip)) {
            strcpy(params->conf.ip, value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "gateway")) {
        if (value_len+1 <= sizeof(params->conf.gateway)) {
            strcpy(params->conf.gateway, value);
        }
        return;
    }
    if (HTTP_CMN_KEYCMP(key, key_len, "ntp")) {
        if (value_len+1 <= sizeof(params->ntp)) {
            strcpy(params->ntp, value);
        }
        return;
    }
}

static esp_err_t http_post_aps_handler(httpd_req_t *req)
{
    json_str_t *json;
    struct wifi_conf params;
    const char *message;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return http_cmn_send_error_json(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.conf.ap.ssid[0] == '\0' || params.ntp[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    err = wifi_conf_set(&params);
    if (err == WIFI_CONF_MISSING_PARAMS) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    if (err == WIFI_CONF_TOO_MANY_ENTRY) {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Too many aps!");
        goto end;
    }
    if (err == WIFI_CONF_UPDATED) {
        message = "Updated";
    } else {
        message = "Added";
    }

    json_str_add_integer(json, "status", 1);
    json_str_begin_string(json, "message", 0);
    json_str_append_string(json, message);
    json_str_append_string(json, " ap '");
    json_str_append_string(json, params.conf.ap.ssid);
    json_str_append_string(json, "'");
    json_str_end_string(json);

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_delete_aps_handler(httpd_req_t *req)
{
    json_str_t *json;
    struct wifi_conf params;
    const char *message;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return http_cmn_send_error_json(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.conf.ap.ssid[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    err = wifi_conf_remove(params.conf.ap.ssid);
    if (err == WIFI_CONF_NOT_FOUND) {
        message = "Unknown";
    } else {
        message = "Removed";
    }

    json_str_add_integer(json, "status", 1);
    json_str_begin_string(json, "message", 0);
    json_str_append_string(json, message);
    json_str_append_string(json, " ap '");
    json_str_append_string(json, params.conf.ap.ssid);
    json_str_append_string(json, "'");
    json_str_end_string(json);

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_get_conn_handler(httpd_req_t *req)
{
    struct simple_wifi_ap_info info;
    if (simple_wifi_get_connection_info(&info) == ESP_OK) {
        json_str_t *json;
        esp_err_t err;

        json = new_json_str(64+SWIFI_SSID_LEN*2);
        if (json == NULL) {
            return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
        }

        json_str_begin_object(json, NULL);
        json_str_add_integer(json, "status", 2);
        json_str_add_string(json, "ssid", info.ssid);
        json_str_add_integer(json, "rssi", info.rssi);
        json_str_add_string(json, "ip", info.ip);
        json_str_begin_string(json, "message", 0);
        json_str_append_string(json, "Connected to '");
        json_str_append_string(json, info.ssid);
        json_str_append_string(json, "'");
        json_str_end_string(json);
        json_str_end_object(json);

        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        err = httpd_resp_sendstr(req, json_str_finalize(json));
        delete_json_str(json);
        return err;
    } else if (simple_wifi_get_connection_state() == SIMPLE_WIFI_CONNECTING) {
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        return httpd_resp_sendstr(req, "{\"status\":1,\"message\":\"Connecting\"}");
    } else {
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        return httpd_resp_sendstr(req, "{\"status\":0,\"message\":\"Not connected\"}");
    }
}

static esp_err_t http_post_conn_handler(httpd_req_t *req)
{
    json_str_t *json;
    struct wifi_conf params;
    const struct wifi_conf *conf;
    int timeout;
    esp_err_t err;

    wifi_conf_load();

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return http_cmn_send_error_json(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return http_cmn_send_error_json(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.conf.ap.ssid[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    conf = wifi_conf_find(params.conf.ap.ssid);
    if (conf == NULL) {
        json_str_add_integer(json, "status", 0);
        json_str_begin_string(json, "message", 0);
        json_str_append_string(json, "Unknown ap '");
        json_str_append_string(json, params.conf.ap.ssid);
        json_str_append_string(json, "'");
        json_str_end_string(json);
        goto end;
    }

    simple_wifi_disconnect();
    simple_wifi_clear_ap();
    ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&conf->conf.ap, sizeof(conf->conf)) );
    timeout = 3;
    while (simple_wifi_get_connection_state() == SIMPLE_WIFI_DISCONNECTING && --timeout > 0) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    if (simple_wifi_get_connection_state() != SIMPLE_WIFI_DISCONNECTED) {
        json_str_add_integer(json, "status", -1);
        json_str_add_string(json, "message", "Failed to disconnect");
        goto end;
    }

    ESP_ERROR_CHECK( simple_wifi_connect_direct(params.conf.ap.ssid) );

    delete_json_str(json);
    return http_get_conn_handler(req);

end:
    json_str_end_object(json);

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    err = httpd_resp_sendstr(req, json_str_finalize(json));
    delete_json_str(json);
    return err;
}

static esp_err_t http_delete_conn_handler(httpd_req_t *req)
{
    const char *json;
    if (simple_wifi_get_connection_state() == SIMPLE_WIFI_CONNECTED) {
        json = "{\"status\":0,\"message\":\"Disconnected\"}";
    } else {
        json = "{\"status\":0,\"message\":\"Not connected\"}";
    }
    simple_wifi_disconnect();
    return httpd_resp_sendstr(req, json);
}

static esp_err_t http_wifi_conf_handler(httpd_req_t *req)
{
    const char *path = req->uri+sizeof(WIFI_CONF_URI)-1;
    size_t path_len = strlen(path);
#define test_path(target_method, target_path) \
    (req->method == target_method && \
        path_len == sizeof(target_path)-1 && strcmp(path, target_path) == 0)

    if (test_path(HTTP_GET, "")) {
        return EMBEDDED_HANDLER_NAME(http_wifi_conf_html)(req);
    }
    if (test_path(HTTP_GET, "/scan")) {
        return http_get_scan_handler(req);
    }
    if (test_path(HTTP_POST, "/scan")) {
        return http_post_scan_handler(req);
    }
    if (test_path(HTTP_GET, "/aps")) {
        return http_get_aps_handler(req);
    }
    if (test_path(HTTP_POST, "/aps")) {
        return http_post_aps_handler(req);
    }
    if (test_path(HTTP_DELETE, "/aps")) {
        return http_delete_aps_handler(req);
    }
    if (test_path(HTTP_GET, "/conn")) {
        return http_get_conn_handler(req);
    }
    if (test_path(HTTP_POST, "/conn")) {
        return http_post_conn_handler(req);
    }
    if (test_path(HTTP_DELETE, "/conn")) {
        return http_delete_conn_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
    return ESP_FAIL;
}

esp_err_t http_wifi_conf_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.uri = WIFI_CONF_URI "*";
    http_uri.handler = http_wifi_conf_handler;
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
    http_uri.method = HTTP_DELETE;
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t http_wifi_conf_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, WIFI_CONF_URI "*", HTTP_GET);
    httpd_unregister_uri_handler(handle, WIFI_CONF_URI "*", HTTP_POST);
    httpd_unregister_uri_handler(handle, WIFI_CONF_URI "*", HTTP_DELETE);
    return ESP_OK;
}
