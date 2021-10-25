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

#pragma once

#include <string.h>
#include <esp_http_server.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_CMN_OK     0
#define HTTP_CMN_FAIL   1

#define HTTP_CMN_ERR_BASE           (0xb000 + 0x100)
#define HTTP_CMN_ERR_INVALID_REQ    (HTTP_CMN_ERR_BASE + 1)
#define HTTP_CMN_ERR_SOCK_TIMEOUT   (HTTP_CMN_ERR_BASE + 2)
#define HTTP_CMN_ERR_TOO_LARGE      (HTTP_CMN_ERR_BASE + 3)


#define EMBEDDED_HANDLER_NAME(name) http_get_ ## name ## _handler
#define MAKE_EMBEDDED_HANDLER(name, content_type) \
    esp_err_t EMBEDDED_HANDLER_NAME(name)(httpd_req_t *req) \
    { \
        extern const unsigned char name ## _start[] asm("_binary_" #name "_start"); \
        extern const unsigned char name ## _end[]   asm("_binary_" #name "_end"); \
        const size_t name ## _size = (name ## _end - name ## _start); \
        httpd_resp_set_type(req, content_type); \
        return httpd_resp_send(req, (const char *)name ## _start, name ## _size); \
    }

extern esp_err_t http_cmn_send_error_json(httpd_req_t *req, esp_err_t err);
extern esp_err_t http_html_cmn_register(httpd_handle_t handle);
extern esp_err_t http_html_cmn_unregister(httpd_handle_t handle);

#define HTTP_CMN_KEYCMP(key, key_len, target)   \
    ((key_len) == sizeof(target)-1 && strcmp((key), target) == 0)

/* Called for each key/value pair in query. value may be NULL if value is
 * ommited in query such as "...&key&..." */
typedef void (*parse_query_handler_t)(char *key, size_t key_len, char *value, size_t value_len, void *user_data);
/* This function modifies query and pass pointer inside the query to handler.
 * You can keep reference to key/value passed to handler as long as you keep
 * reference to the query. */
extern esp_err_t http_cmn_parse_query(char *query, parse_query_handler_t handler, void *user_data);

extern int http_cmn_is_form_data(httpd_req_t *req);
extern int http_cmn_handle_form_data(httpd_req_t *req, parse_query_handler_t handler, void *user_data);

static inline int http_cmn_is_true_like(const char *value)
{
    return strcmp(value, "1") == 0 || strcmp(value, "y") == 0 || strcmp(value, "t") == 0 ||
        strcmp(value, "yes") == 0 || strcmp(value, "true") == 0 || strcmp(value, "on") == 0;
}

static inline int http_cmn_is_false_like(const char *value)
{
    return strcmp(value, "0") == 0 || strcmp(value, "n") == 0 || strcmp(value, "f") == 0 ||
        strcmp(value, "no") == 0 || strcmp(value, "false") == 0 || strcmp(value, "off") == 0;
}

#ifdef __cplusplus
}
#endif
