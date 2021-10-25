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
#include <esp_err.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include "http_html_cmn.h"

#include "queryparser.h"

#define TAG "html_cmn"

#define FORM_TYPE   "application/x-www-form-urlencoded"
#define MAX_QUERY_SIZE  (4*1024)

static MAKE_EMBEDDED_HANDLER(cmn_js, "application/javascript")
static MAKE_EMBEDDED_HANDLER(cmn_css, "text/css")

esp_err_t http_cmn_send_error_json(httpd_req_t *req, esp_err_t err)
{
    const char *status;
    const char *msg;
    if (err == HTTP_CMN_ERR_INVALID_REQ) {
        status = "400 Bad Request";
        msg = "{\"status\":-1,\"message\":\"Bad Request\"}";
    } else if (err == HTTP_CMN_ERR_SOCK_TIMEOUT) {
        status = "408 Request Timeout";
        msg = "{\"status\":-1,\"message\":\"Server closed this connection\"}";
    } else if (err == HTTP_CMN_ERR_TOO_LARGE) {
        status = "413 Entity Too Large";
        msg = "{\"status\":-1,\"message\":\"Entity is too large\"}";
    } else if (err == ESP_ERR_NOT_SUPPORTED) {
        status = "404 Not Found";
        msg = "{\"status\":-1,\"message\":\"Unsupported request\"}";
    } else {
        status = "500 Internal Server Error";
        msg = "{\"status\":-1,\"message\":\"Server Error\"}";
    }
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, msg);
    return ESP_FAIL;
}

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


bool http_cmn_req_is_form_data(httpd_req_t *req)
{
    char content_type[sizeof(FORM_TYPE)+4];
    esp_err_t err;

    err = httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Content-Type: %s", content_type);
    }
    return (err == ESP_OK && strcmp(content_type, FORM_TYPE) == 0);
}

struct query_parser_bridge {
    parse_query_handler_t handler;
    void *user_data;
};

static void queryparser_bridge_handler(queryparser_t *parser, char *key, size_t key_len, char *value, size_t value_len, void *user_data)
{
    struct query_parser_bridge *bridge = user_data;
    (void)parser;
    bridge->handler(key, key_len, value, value_len, bridge->user_data);
}

esp_err_t http_cmn_parse_query(char *query, parse_query_handler_t handler, void *user_data)
{
    struct query_parser_bridge bridge = {handler, user_data};
    int ret;
    if (query == NULL || handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ret = queryparser_parse(query, queryparser_bridge_handler, &bridge);
    if (ret < 0) {
        if (ret == QUERY_PARSER_MALFORMED) {
            return HTTP_CMN_ERR_INVALID_REQ;
        } else {
            return HTTP_CMN_FAIL;
        }
    }
    return HTTP_CMN_OK;
}

esp_err_t http_cmn_handle_form_data(httpd_req_t *req, parse_query_handler_t handler, void *user_data)
{
    struct query_parser_bridge bridge = {handler, user_data};
    queryparser_t *parser;
    size_t remaining_size, size;
    void *scratch;
    int ret;
    esp_err_t err;

    if (req == NULL || handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!http_cmn_req_is_form_data(req)) {
        return HTTP_CMN_ERR_INVALID_REQ;
    }
    if (req->content_len == 0) {
        return HTTP_CMN_OK;
    }
    if (req->content_len > MAX_QUERY_SIZE) {
        return HTTP_CMN_ERR_TOO_LARGE;
    }

    /* this library does not expect to receive large data. */
    parser = queryparser_new(MIN(1024, req->content_len+1), queryparser_bridge_handler, &bridge);
    if (parser == NULL) {
        return HTTP_CMN_ERR_TOO_LARGE;
    }

    remaining_size = req->content_len;
    while (remaining_size > 0) {
        queryparser_get_scratch(parser, &scratch, &size);
        if (size < 80 && size < remaining_size) {
            free(parser);
            return HTTP_CMN_ERR_TOO_LARGE;
        }
        ret = httpd_req_recv(req, scratch, MIN(size, remaining_size));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                err = HTTP_CMN_ERR_SOCK_TIMEOUT;
            } else {
                err = HTTP_CMN_FAIL;
            }
            free(parser);
            return err;
        }
        remaining_size -= ret;
        ret = queryparser_update(parser, ret);
        if (ret == QUERY_PARSER_MALFORMED) {
            free(parser);
            return HTTP_CMN_FAIL;
        }
    }
    ret = queryparser_finish(parser);
    free(parser);

    if (ret < 0) {
        if (ret == QUERY_PARSER_MALFORMED) {
            return HTTP_CMN_ERR_INVALID_REQ;
        } else {
            return HTTP_CMN_FAIL;
        }
    }
    return HTTP_CMN_OK;
}
