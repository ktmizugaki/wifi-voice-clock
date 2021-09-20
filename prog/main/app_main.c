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
#include <string.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include <esp_err.h>
#include <esp_log.h>

#include <simple_wifi.h>
#include <http_wifi_conf.h>
#include <lan_manager.h>
#include <clock.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_display.h"
#include "app_mode.h"

#define TAG "main"

static app_mode_t s_mode = APP_MODE_INITIAL;

static esp_err_t app_init_nvs(void)
{
#if NVS_KEY_SIZE != 32
#error unsupported NVS_KEY_SIZE
#endif
    extern const uint8_t nvskey_dat[] asm("_binary_nvskey_dat_start");
    esp_err_t err;
    nvs_sec_cfg_t cfg;
    memcpy(&cfg, nvskey_dat, NVS_KEY_SIZE*2);

    /* this is not secure without flash encryption */
    err = nvs_flash_secure_init(&cfg);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_secure_init(&cfg);
    }
    return err;
}

extern app_mode_t app_mode_get_current(void)
{
    return s_mode;
}

void app_main(void)
{
    ESP_LOGD(TAG, "Started");

    ESP_ERROR_CHECK( app_display_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( app_event_init() );
    ESP_ERROR_CHECK( app_switches_init() );
    ESP_ERROR_CHECK( app_clock_init() );
    ESP_ERROR_CHECK( simple_wifi_init() );
    ESP_ERROR_CHECK( lan_manager_init() );
    ESP_ERROR_CHECK( app_init_nvs() );

    setenv("TZ", "JST-9", 1);
    tzset();

    if (!http_wifi_conf_configured()) {
        s_mode = APP_MODE_INITIAL;
    } else if (!clock_is_valid()) {
        s_mode = APP_MODE_INITIALSYNC;
    } else {
        s_mode = APP_MODE_CLOCK;
    }
    while (true) {
        switch (s_mode) {
        case APP_MODE_INITIAL: s_mode = app_mode_initial(); break;
        case APP_MODE_INITIALSYNC: s_mode = app_mode_initialsync(); break;
        case APP_MODE_CLOCK: s_mode = app_mode_clock(); break;
        case APP_MODE_SETTINGS: s_mode = app_mode_settings(); break;
        }
    }
}
