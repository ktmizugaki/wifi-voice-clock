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

#define TAG "wifi_conf"
#define WIFI_CONF_URI  "/wifi_conf"

static MAKE_EMBEDDED_HANDLER(http_wifi_conf_html, "text/html")

/* from simple_wifi/simple_sta.c */
#ifdef CONFIG_SWIFI_MAX_AP_CONFS
#define SWIFI_MAX_AP_CONFS  CONFIG_SWIFI_MAX_AP_CONFS
#else
#define SWIFI_MAX_AP_CONFS  3
#endif

struct wifi_conf_state {
    bool valid;
    bool changed;
};
struct wifi_conf {
    struct simple_wifi_ap_static_conf conf;
    char ntp[WIFI_CONF_NTP_SERVER_LENGTH];
};

static int8_t s_num_wifi_conf = -1;
static struct wifi_conf_state s_wifi_conf_states[SWIFI_MAX_AP_CONFS];
static struct wifi_conf s_wifi_confs[SWIFI_MAX_AP_CONFS];

static bool s_waiting_scan_done = false;

static esp_err_t wifi_conf_load(void)
{
    char name[8] = "conf00";
    esp_err_t err;
    nvs_handle_t nvsh;
    size_t length;
    int i, n;

    if (s_num_wifi_conf >= 0) {
        return ESP_OK;
    }

    err = nvs_open("wifi_conf", NVS_READONLY, &nvsh);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        s_num_wifi_conf = 0;
        for (i = 0, n = 0; i < SWIFI_MAX_AP_CONFS; i++) {
            s_wifi_conf_states[i].valid = false;
            s_wifi_conf_states[i].changed = false;
        }
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0, n = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        name[3] = '0'+(i/10);
        name[4] = '0'+(i%10);
        length = sizeof(struct wifi_conf);
        err = nvs_get_blob(nvsh, name, &s_wifi_confs[i], &length);
        if (err == ESP_OK && length == sizeof(struct wifi_conf)) {
            s_wifi_conf_states[i].valid = true;
            s_wifi_conf_states[i].changed = false;
            n++;
        } else {
            s_wifi_conf_states[i].valid = false;
            s_wifi_conf_states[i].changed = false;
        }
    }
    s_num_wifi_conf = n;
    nvs_close(nvsh);

    return err;
}

static esp_err_t wifi_conf_save(void)
{
    char name[8] = "conf00";
    esp_err_t err;
    nvs_handle_t nvsh;
    int i;

    err = nvs_open("wifi_conf", NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        name[3] = '0'+(i/10);
        name[4] = '0'+(i%10);
        if (!s_wifi_conf_states[i].changed) {
            err = ESP_OK;
        } else if (s_wifi_conf_states[i].valid) {
            err = nvs_set_blob(nvsh, name, &s_wifi_confs[i], sizeof(struct wifi_conf));
        } else {
            err = nvs_erase_key(nvsh, name);
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                err = ESP_OK;
            }
        }
        if (err != ESP_OK) {
            goto end;
        }
        s_wifi_conf_states[i].changed = false;
    }

    err = nvs_commit(nvsh);

end:
    nvs_close(nvsh);
    return err;
}

static int find_wifi_conf(const char *ssid)
{
    int i;
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        if (s_wifi_conf_states[i].valid) {
            if (strcmp(s_wifi_confs[i].conf.ap.ssid, ssid) == 0) {
                return i;
            }
        }
    }
    return -1;
}

static int find_unsued_wifi_conf(void)
{
    int i;
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        if (!s_wifi_conf_states[i].valid) {
            return i;
        }
    }
    return -1;
}

static esp_err_t wifi_conf_send_error(httpd_req_t *req, esp_err_t err)
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
        return wifi_conf_send_error(req, HTTP_CMN_FAIL);
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

    json = new_json_str(16+sizeof(struct wifi_conf)*s_num_wifi_conf);
    if (json == NULL) {
        return wifi_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);
    json_str_add_integer(json, "status", 1);

    json_str_begin_array(json, "aps");
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        struct wifi_conf *conf = &s_wifi_confs[i];
        if (!s_wifi_conf_states[i].valid) {
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
    int conf_index;
    const char *message;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return wifi_conf_send_error(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return wifi_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.conf.ap.ssid[0] == '\0' || params.ntp[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    conf_index = find_wifi_conf(params.conf.ap.ssid);
    if (conf_index == -1 && params.conf.ap.password[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    if (params.conf.ap.use_static_ip) {
        if (params.conf.ip[0] == '\0' || params.conf.gateway[0] == '\0') {
            json_str_add_integer(json, "status", 0);
            json_str_add_string(json, "message", "Missing params");
            goto end;
        }
        if (params.conf.netmask[0] == '\0') {
            strcpy(params.conf.netmask, "255.255.255.0");
        }
    }
    if (conf_index == -1) {
        conf_index = find_unsued_wifi_conf();
        if (conf_index == -1) {
            json_str_add_integer(json, "status", 0);
            json_str_add_string(json, "message", "Too many aps!");
            goto end;
        }
        message = "Added";
        s_num_wifi_conf++;
    } else {
        message = "Updated";
    }
    s_wifi_conf_states[conf_index].valid = true;
    s_wifi_conf_states[conf_index].changed = true;
    if (params.conf.ap.password[0] == '\0') {
        strcpy(params.conf.ap.password, s_wifi_confs[conf_index].conf.ap.password);
    }
    s_wifi_confs[conf_index] = params;
    wifi_conf_save();

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
    int conf_index;
    const char *message;
    esp_err_t err;

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return wifi_conf_send_error(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return wifi_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    if (params.conf.ap.ssid[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    conf_index = find_wifi_conf(params.conf.ap.ssid);
    if (conf_index == -1) {
        message = "Unknown";
    } else {
        message = "Removed";
        s_num_wifi_conf--;
        s_wifi_conf_states[conf_index].valid = false;
        s_wifi_conf_states[conf_index].changed = true;
        wifi_conf_save();
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
            return wifi_conf_send_error(req, HTTP_CMN_FAIL);
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
    int conf_index;
    int timeout;
    esp_err_t err;

    wifi_conf_load();

    memset(&params, 0, sizeof(params));
    err = http_cmn_handle_form_data(req, post_aps_params_handler, &params);
    if (err != HTTP_CMN_OK) {
        return wifi_conf_send_error(req, err);
    }

    json = new_json_str(64);
    if (json == NULL) {
        return wifi_conf_send_error(req, HTTP_CMN_FAIL);
    }

    json_str_begin_object(json, NULL);

    conf_index = find_wifi_conf(params.conf.ap.ssid);
    if (params.conf.ap.ssid[0] == '\0') {
        json_str_add_integer(json, "status", 0);
        json_str_add_string(json, "message", "Missing params");
        goto end;
    }
    if (conf_index == -1) {
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
    ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&s_wifi_confs[conf_index].conf.ap, sizeof(s_wifi_confs[conf_index].conf)) );
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

bool http_wifi_conf_configured(void)
{
    wifi_conf_load();
    return s_num_wifi_conf != 0;
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == SIMPLE_WIFI_EVENT) {
        switch(event_id) {
        case SIMPLE_WIFI_EVENT_SCAN_DONE:
            if (s_waiting_scan_done) {
                if (simple_wifi_is_scan_result_available()) {
                    simple_wifi_connect();
                }
                s_waiting_scan_done = false;
            }
            break;
        }
    }
}

esp_err_t http_wifi_conf_connect(void)
{
    int i;
    int timeout;
    esp_err_t err;

    if (simple_wifi_get_connection_state() == SIMPLE_WIFI_CONNECTED) {
        return ESP_OK;
    }

    wifi_conf_load();

    if (s_num_wifi_conf == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    simple_wifi_disconnect();
    simple_wifi_clear_ap();
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        struct wifi_conf *conf = &s_wifi_confs[i];
        if (!s_wifi_conf_states[i].valid) {
            continue;
        }
        ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&conf->conf.ap, sizeof(conf->conf)) );
    }

    timeout = 30;
    if (simple_wifi_get_connection_state() == SIMPLE_WIFI_DISCONNECTING && --timeout > 0) {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    if (timeout == 0) {
        ESP_LOGW(TAG, "Failed to disconnect");
        return ESP_ERR_TIMEOUT;
    }

    if (!simple_wifi_is_scan_result_available()) {
        s_waiting_scan_done = true;
        /* overwrites previous registration if exists */
        ESP_ERROR_CHECK( esp_event_handler_register(
            SIMPLE_WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
        simple_wifi_scan();
        return ESP_OK;
    }

    err = simple_wifi_connect();
    return err == ESP_OK? ESP_OK: ESP_FAIL;
}

esp_err_t http_wifi_conf_connect_direct(const char *ssid)
{
    int conf_index;
    int timeout;
    esp_err_t err;

    wifi_conf_load();

    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_num_wifi_conf == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    conf_index = find_wifi_conf(ssid);
    if (conf_index == -1) {
        return ESP_ERR_NOT_FOUND;
    }

    simple_wifi_disconnect();
    simple_wifi_clear_ap();
    ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&s_wifi_confs[conf_index].conf.ap, sizeof(s_wifi_confs[conf_index].conf)) );
    timeout = 30;
    while (simple_wifi_get_connection_state() == SIMPLE_WIFI_DISCONNECTING && --timeout > 0) {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    if (simple_wifi_get_connection_state() != SIMPLE_WIFI_DISCONNECTED) {
        return ESP_ERR_TIMEOUT;
    }

    err = simple_wifi_connect_direct(ssid);
    return err == ESP_OK? ESP_OK: ESP_FAIL;
}

esp_err_t http_wifi_conf_get_ntp(char ntp[WIFI_CONF_NTP_SERVER_LENGTH])
{
    struct simple_wifi_ap_info info;
    int conf_index;
    esp_err_t err;

    err = simple_wifi_get_connection_info(&info);
    if (err != ESP_OK) {
        return err;
    }

    conf_index = find_wifi_conf(info.ssid);
    if (conf_index == -1) {
        return ESP_ERR_NOT_FOUND;
    }
    strcpy(ntp, s_wifi_confs[conf_index].ntp);

    return ESP_OK;
}
