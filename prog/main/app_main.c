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
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include <esp_log.h>


#define TAG "main"

enum app_mode {
    APP_MODE_INITIAL,
};

static enum app_mode s_mode = APP_MODE_INITIAL;

static enum app_mode handle_initial(void)
{
    while (true) {
        ESP_LOGD(TAG, "handle_initial");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
    return APP_MODE_INITIAL;
}

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

void app_main(void)
{
    ESP_LOGD(TAG, "Started");

    ESP_ERROR_CHECK( app_init_nvs() );

    setenv("TZ", "JST-9", 1);
    tzset();

    while (true) {
        switch (s_mode) {
        case APP_MODE_INITIAL: s_mode = handle_initial(); break;
        }
    }
}
