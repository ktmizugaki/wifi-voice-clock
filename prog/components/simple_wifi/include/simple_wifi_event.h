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
#include <esp_event.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIMPLE_WIFI_EVENT_SCAN_DONE,                /**< ESP32 finish scanning AP */
    SIMPLE_WIFI_EVENT_STA_CONNECTED,            /**< ESP32 station connected to AP */
    SIMPLE_WIFI_EVENT_STA_DISCONNECTED,         /**< ESP32 station disconnected from AP */
    SIMPLE_WIFI_EVENT_STA_FAIL,                 /**< ESP32 station failed to connect to AP */

    SIMPLE_WIFI_EVENT_SOFTAP_START,             /**< ESP32 soft-AP start */
    SIMPLE_WIFI_EVENT_SOFTAP_STOP,              /**< ESP32 soft-AP stop */
} simple_wifi_event_t;

ESP_EVENT_DECLARE_BASE(SIMPLE_WIFI_EVENT);

#ifdef __cplusplus
}
#endif
