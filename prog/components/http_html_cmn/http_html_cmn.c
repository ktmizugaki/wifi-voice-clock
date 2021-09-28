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

esp_err_t http_cmn_parse_query(char *query, parse_query_handler_t handler, void *user_data)
{
    if (query == NULL || handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    while (1) {
        char *key, *value;
        size_t key_len, value_len;
        key = strtok_r(NULL, "&", &query);
        if (key == NULL) {
            break;
        }
        value = strchr(key, '=');
        if (value == NULL) { /* '=' is missing: key only segment */
            if (query == NULL) {
                key_len = strlen(key);
            } else if (query[-1] != '\0') {
                key_len = query-key;
            } else {
                key_len = query-1-key;
            }
            value_len = 0;
        } else {
            key_len = value-key;
            *value++ = '\0';
            if (query == NULL) {
                value_len = strlen(value);
            } else if (query[-1] != '\0') {
                value_len = query-value;
            } else {
                value_len = query-1-value;
            }
        }
        /* TODO: decode urlencoded chars */
        ESP_LOGD(TAG, "%s(%zu)=%s(%zu)", key?key:"(null)", key_len, value?value:"(null)", value_len);
        handler(key, key_len, value, value_len, user_data);
    }
    return ESP_OK;
}

esp_err_t http_cmn_handle_form_data(httpd_req_t *req, parse_query_handler_t handler, void *user_data)
{
#define FORM_TYPE   "application/x-www-form-urlencoded"
    char content_type[sizeof(FORM_TYPE)+4];
    char *query;
    size_t off;
    int ret;
    esp_err_t err;

    err = httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Content-Type: %s", content_type);
    }
    if (err != ESP_OK || strcmp(content_type, FORM_TYPE) != 0) {
        return HTTP_CMN_ERR_INVALID_REQ;
    }
    if (req->content_len == 0) {
        return HTTP_CMN_OK;
    }
    query = malloc(req->content_len + 1);
    off = 0;
    if (query == NULL) {
        return HTTP_CMN_FAIL;
    }
    while (off < req->content_len) {
        ret = httpd_req_recv(req, query + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                ret = HTTP_CMN_ERR_SOCK_TIMEOUT;
            } else {
                ret = HTTP_CMN_FAIL;
            }
            free (query);
            return ret;
        }
        off += ret;
    }
    query[off] = '\0';

    err = http_cmn_parse_query(query, handler, user_data);
    free(query);
    return HTTP_CMN_OK;
}
