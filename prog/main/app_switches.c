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

#include <switches.h>

#include "app_switches.h"

#define TAG "switches"

#define SWITCH_1        GPIO_NUM_34     /* ESPr pin 19 */
#define SWITCH_2        GPIO_NUM_35     /* ESPr pin 18 */
#define SWITCH_3        GPIO_NUM_13
#define SWITCH_MASK     (BIT64(SWITCH_1)|BIT64(SWITCH_2)|BIT64(SWITCH_3))
#define NUM_SWITCHES    3

static enum app_action s_last_action = APP_ACTION_NONE;

static void app_switches_callback(enum switch_flags action, enum switch_flags prev_state)
{
    ESP_LOGI(TAG, "action: %#x, prev_state: %#x", action, prev_state);
    s_last_action = (enum app_action)action;
}

void app_switches_enable_wake(void)
{
    esp_sleep_enable_ext1_wakeup(BIT64(SWITCH_1)|BIT64(SWITCH_2), ESP_EXT1_WAKEUP_ANY_HIGH);
}

int app_switches_check_wake(void)
{
    uint64_t wakeup_bit = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_bit & BIT64(SWITCH_1)) {
        ESP_LOGI(TAG, "woken up by switch 1");
        return APP_ACTION_RIGHT;
    }
    if (wakeup_bit & BIT64(SWITCH_2)) {
        ESP_LOGI(TAG, "woken up by switch 2");
        return APP_ACTION_LEFT;
    }
    return APP_ACTION_NONE;
}

esp_err_t app_switches_init(void)
{
    esp_err_t ret;
    static const uint8_t pins[NUM_SWITCHES] = {
        SWITCH_1,
        SWITCH_2,
        SWITCH_3,
    };
    ret = switches_init(app_switches_callback, NUM_SWITCHES, pins);
    if (ret != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}

void app_switches_wait_up(void)
{
    /* wait switchess are up */
    while (gpio_get_level(SWITCH_1) || gpio_get_level(SWITCH_2)) {
        ESP_LOGD(TAG, "wait up: %d,%d", gpio_get_level(SWITCH_1), gpio_get_level(SWITCH_2));
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    vTaskDelay(50/portTICK_PERIOD_MS);
}

enum app_action app_switches_get_action(void)
{
    enum app_action action = s_last_action;
    s_last_action = APP_ACTION_NONE;
    return action;
}
