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

#include "http_html_cmn.h"

#define TAG "html_cmn"

static MAKE_EMBEDDED_HANDLER(cmn_js, "application/javascript")
static MAKE_EMBEDDED_HANDLER(cmn_css, "text/css")

esp_err_t http_html_cmn_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.method = HTTP_GET;
    http_uri.user_ctx = NULL;

    http_uri.uri = "/cmn.js";
    http_uri.handler = EMBEDDED_HANDLER_NAME(cmn_js);
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    http_uri.uri = "/cmn.css";
    http_uri.handler = EMBEDDED_HANDLER_NAME(cmn_css);
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t http_html_cmn_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, "/cmn.js", HTTP_GET);
    httpd_unregister_uri_handler(handle, "/cmn.css", HTTP_GET);
    return ESP_OK;
}
