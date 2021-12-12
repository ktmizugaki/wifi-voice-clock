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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include "simple_wifi.h"
#include "simple_wifi_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SWIFI_IFNAME_PREFIX
#define SWIFI_IFNAME_PREFIX CONFIG_SWIFI_IFNAME_PREFIX
#else
#define SWIFI_IFNAME_PREFIX "esp32"
#endif

extern enum simple_wifi_mode simple_wifi_mode;
extern enum simple_wifi_connection_state simple_connection_state;
extern int simple_sta_retry;

#define swifi_event_post(event, data, size) \
    esp_event_post(SIMPLE_WIFI_EVENT, SIMPLE_WIFI_EVENT_ ## event, \
        data, size, 0)

static inline int swifi_time(void)
{
    return xTaskGetTickCount()/(1000/portTICK_PERIOD_MS);
}

extern bool simple_wifi__lock(TickType_t ticks_to_wait);
extern void simple_wifi__unlock(void);

extern esp_err_t simple_sta_init(void);
extern esp_err_t simple_sta_set_scan_result(void);
extern void simple_sta_clear_scan_result(void);
extern void simple_sta_stop(void);

extern esp_err_t simple_softap_configure(void);
extern void simple_softap_stop(void);

#ifdef __cplusplus
}
#endif
