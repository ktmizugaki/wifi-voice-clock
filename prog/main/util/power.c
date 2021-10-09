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

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <esp_log.h>

#include <alarm.h>
#include <clock_conf.h>
#include <vcc.h>
#include "app_display.h"
#include "app_switches.h"
#include "power.h"

#define TAG "power"

static int64_t calc_wakup_us(void)
{
    static int64_t (*const functions[])(struct tm *tm, suseconds_t *usec) = {
        clock_conf_wakeup_us,
        alarm_wakeup_us,
        NULL
    };
    int64_t wakeup_us, us;
    struct timeval tv;
    struct tm tm;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    /* wake up at least once in 24 hour. */
    wakeup_us = 24*3600*1000000LL;
    int i;
    for (i = 0; functions[i]; i++) {
        us = functions[i](&tm, &tv.tv_usec);
        ESP_LOGD(TAG, "wakeup_us[%d]: %d.%06d", i, (int)(us/1000000LLU), (int)(us%1000000LLU));
        if (wakeup_us > us) wakeup_us = us;
    }
    ESP_LOGD(TAG, "wakeup_us: %d.%06d", (int)(wakeup_us/1000000LLU), (int)(wakeup_us%1000000LLU));
    return wakeup_us;
}

esp_err_t power_init(void)
{
    int vcc;
    vcc_charge_state_t state;
    esp_err_t err;

    err = vcc_init();
    if (err != ESP_OK) {
        return err;
    }
    err = vcc_read(&vcc, true);
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT) {
        return err;
    }
    err = vcc_get_charge_state(&state);
    if (err != ESP_OK) {
        return err;
    }
    ESP_LOGI(TAG, "vcc: %d.%03d, %d, charge: %d",
        vcc/1000, vcc%1000, vcc_get_level(false), state);
    return ESP_OK;
}

void power_suspend(void)
{
    ESP_LOGI(TAG, "suspend");
    app_display_off();
    app_switches_wait_up();

    esp_sleep_enable_timer_wakeup(calc_wakup_us());
    app_switches_enable_wake();
    esp_deep_sleep_start();
}

void power_hibernate(void)
{
    ESP_LOGI(TAG, "hibernate");
    app_display_off();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    app_switches_enable_wake();
    esp_deep_sleep_start();
}

void power_halt(void)
{
    ESP_LOGI(TAG, "halt");
    app_display_off();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
}
