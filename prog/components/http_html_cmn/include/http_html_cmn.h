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

#include <esp_http_server.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

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

extern esp_err_t http_html_cmn_register(httpd_handle_t handle);
extern esp_err_t http_html_cmn_unregister(httpd_handle_t handle);

#ifdef __cplusplus
}
#endif
