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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <esp_err.h>
#include <esp_log.h>

#include <simple_wifi.h>
#include <simple_wifi_event.h>
#include <lan_manager.h>

#include "app_wifi.h"
#include "app_event.h"

#define TAG "wifi"

#define SWITCH_1        GPIO_NUM_34     /* ESPr pin 19 */
#define SWITCH_2        GPIO_NUM_35     /* ESPr pin 18 */
#define SWITCH_3        GPIO_NUM_13
#define SWITCH_MASK     (BIT64(SWITCH_1)|BIT64(SWITCH_2)|BIT64(SWITCH_3))
#define NUM_SWITCHES    3

static bool s_initialized = false;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == SIMPLE_WIFI_EVENT) {
        app_event_send_arg(APP_EVENT_WIFI, event_id);
    }
}

esp_err_t app_wifi_ensure_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    s_initialized = true;
    ESP_ERROR_CHECK( simple_wifi_init() );
    ESP_ERROR_CHECK( lan_manager_init() );
    ESP_ERROR_CHECK( esp_event_handler_register(SIMPLE_WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
    return ESP_OK;
}
