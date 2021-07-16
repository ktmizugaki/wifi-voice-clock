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
#include <esp_err.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include <simple_wifi.h>
#include <http_html_cmn.h>

#include "http_wifi_conf.h"

static MAKE_EMBEDDED_HANDLER(http_wifi_conf_html, "text/html")

#define TAG "wifi_conf"

static esp_err_t http_get_scan_handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "FIXME: stub %s", __func__);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Implemented");
    return ESP_OK;
}

static esp_err_t http_post_scan_handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "FIXME: stub %s", __func__);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Implemented");
    return ESP_OK;
}

static esp_err_t http_get_aps_handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "FIXME: stub %s", __func__);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Implemented");
    return ESP_OK;
}

static esp_err_t http_post_aps_handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "FIXME: stub %s", __func__);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Implemented");
    return ESP_OK;
}

static esp_err_t http_delete_aps_handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "FIXME: stub %s", __func__);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Implemented");
    return ESP_OK;
}

static esp_err_t http_wifi_conf_handler(httpd_req_t *req)
{
    const char *path = req->uri+sizeof("/wifi_conf")-1;
    if (req->method == HTTP_GET && strcmp(path, "") == 0) {
        return EMBEDDED_HANDLER_NAME(http_wifi_conf_html)(req);
    }
    if (req->method == HTTP_GET && strcmp(path, "/scan") == 0) {
        return http_get_scan_handler(req);
    }
    if (req->method == HTTP_POST && strcmp(path, "/scan") == 0) {
        return http_post_scan_handler(req);
    }
    if (req->method == HTTP_GET && strcmp(path, "/aps") == 0) {
        return http_get_aps_handler(req);
    }
    if (req->method == HTTP_POST && strcmp(path, "/aps") == 0) {
        return http_post_aps_handler(req);
    }
    if (req->method == HTTP_DELETE && strcmp(path, "/aps") == 0) {
        return http_delete_aps_handler(req);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
    return ESP_FAIL;
}

esp_err_t http_wifi_conf_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.uri = "/wifi_conf*";
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
    httpd_unregister_uri_handler(handle, "/cmn.js", HTTP_GET);
    httpd_unregister_uri_handler(handle, "/cmn.css", HTTP_GET);
    return ESP_OK;
}
