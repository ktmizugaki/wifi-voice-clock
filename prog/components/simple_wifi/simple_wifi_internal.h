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
#include "simple_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern enum simple_wifi_mode simple_wifi_mode;
extern enum simple_wifi_connection_state simple_connection_state;

extern esp_err_t simple_sta_set_scan_result(void);
extern void simple_sta_clear_scan_result(void);
extern void simple_sta_stop(void);

extern esp_err_t simple_softap_configure(void);
extern void simple_softap_stop(void);

#ifdef __cplusplus
}
#endif
