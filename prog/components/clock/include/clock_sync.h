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

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * synchronize clock with ntp server.
 */

/**
 * @brief start sntp process.
 * @note device must be connected to network before calling this function.
 * @param[in] ntp_server    address of ntp server to synchronize with.
 * @return ESP_OK when successfully started sntp process.
 */
extern esp_err_t clock_sync_sntp_start(const char *ntp_server);
/**
 * @brief stop sntp process.
 * @return ESP_OK.
 */
extern esp_err_t clock_sync_sntp_stop(void);

#ifdef __cplusplus
}
#endif
