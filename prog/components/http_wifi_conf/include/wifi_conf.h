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

/**
 * @file
 * wifi connection conf.
 */

/** max length of ntp server address. */
#define WIFI_CONF_NTP_SERVER_LENGTH 22

/**
 * @brief check if wifi conf is configured.
 * @return true if wifi conf is configured.
 */
extern bool wifi_conf_configured(void);
/**
 * @brief try to connect to any configured SSID.
 * @return ESP_OK if starting to connect.
 */
extern esp_err_t wifi_conf_connect(void);
/**
 * @brief try to connect to specified SSID.
 * @return ESP_OK if starting to connect.
 */
extern esp_err_t wifi_conf_connect_direct(const char *ssid);

/**
 * @brief get configured ntp server for current SSID.
 * copy ntp server to buffer if connected to wifi and ntp server is configured
 * for connected SSID.
 * @param[out] ntp  buffer to set ntp address, should be at least @ref WIFI_CONF_NTP_SERVER_LENGTH.
 * @return
 *  - ESP_OK when stored ntp server to ntp.
 *  - ESP_ERR_WIFI_NOT_CONNECT if not connected to wifi.
 */
extern esp_err_t wifi_conf_get_ntp(char ntp[WIFI_CONF_NTP_SERVER_LENGTH]);

#ifdef __cplusplus
}
#endif
