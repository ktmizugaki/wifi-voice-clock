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
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_err.h>
#include <esp_log.h>

#include <simple_wifi.h>
#include <http_html_cmn.h>
#include <http_wifi_conf.h>

#define TAG "main"

enum app_mode {
    APP_MODE_INITIAL,
};

static enum app_mode s_mode = APP_MODE_INITIAL;

static void start_initial_httpd(httpd_handle_t *httpd)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 32; /* default 8 will be too small */
    config.stack_size = 10*1024;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(httpd, &config) != ESP_OK) {
        ESP_LOGW(TAG, "Error starting server");
        return;
    }

    ESP_ERROR_CHECK( http_html_cmn_register(*httpd) );
    ESP_ERROR_CHECK( http_wifi_conf_register(*httpd) );
}

static enum app_mode handle_initial(void)
{
    httpd_handle_t httpd = NULL;
    char ssid[SWIFI_SSID_LEN];
    char password[SWIFI_PW_LEN];
    ESP_LOGD(TAG, "handle_initial");

    simple_wifi_clear_ap();
    simple_wifi_start(SIMPLE_WIFI_MODE_STA_SOFTAP);

    simple_wifi_get_ssid(ssid);
    simple_wifi_get_password(password);
    ESP_LOGI(TAG, "SoftAP: SSID=%s, password=%s", ssid, password);

    start_initial_httpd(&httpd);

    while (true) {
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }

    httpd_stop(httpd);
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
