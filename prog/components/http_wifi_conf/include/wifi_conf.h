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

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_CONF_NTP_SERVER_LENGTH 22

extern bool wifi_conf_configured(void);
extern esp_err_t wifi_conf_connect(void);
extern esp_err_t wifi_conf_connect_direct(const char *ssid);

extern esp_err_t wifi_conf_get_ntp(char ntp[WIFI_CONF_NTP_SERVER_LENGTH]);

#ifdef __cplusplus
}
#endif
