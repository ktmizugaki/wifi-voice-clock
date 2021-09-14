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
#include <http_alarm_conf.h>
#include <lan_manager.h>
#include <clock.h>

#include "app_clock.h"
#include "app_switches.h"

#define TAG "main"

enum app_mode {
    APP_MODE_INITIAL,
    APP_MODE_INITIALSYNC,
    APP_MODE_CLOCK,
    APP_MODE_SETTINGS,
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

static void start_settings_httpd(httpd_handle_t *httpd)
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
    ESP_ERROR_CHECK( http_alarm_conf_register(*httpd) );
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
    return http_wifi_conf_configured()? APP_MODE_INITIALSYNC: APP_MODE_INITIAL;
}

static enum app_mode handle_initialsync(void)
{
    esp_err_t err;
    ESP_LOGD(TAG, "handle_initialsync");

    err = app_clock_start_sync();
    if (err != ESP_OK) {
        return APP_MODE_INITIAL;
    }

    while (true) {
        if (app_clock_is_done()) {
            app_clock_stop_sync();
            return clock_is_valid()? APP_MODE_CLOCK: APP_MODE_INITIAL;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

static enum app_mode handle_clock(void)
{
    enum app_action action;
    ESP_LOGD(TAG, "handle_clock");

    while (true) {
        struct tm tm;
        char buf_date[11], buf_time[11];
        clock_localtime(&tm);
        strftime(buf_date, sizeof(buf_date), "%m/%d %a", &tm);
        strftime(buf_time, sizeof(buf_time), "%H:%M:%S", &tm);
        printf("Time is: %s %s\n", buf_date, buf_time);
        action = app_switches_get_action();
        if (action == (APP_ACTION_MIDDLE|APP_ACTION_FLAG_RELEASE)) {
            return APP_MODE_SETTINGS;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    return APP_MODE_CLOCK;
}

static enum app_mode handle_settings(void)
{
    enum app_action action;
    httpd_handle_t httpd = NULL;
    char ssid[SWIFI_SSID_LEN];
    char password[SWIFI_PW_LEN];
    ESP_LOGD(TAG, "handle_settings");

    lan_manager_request_softap();

    simple_wifi_get_ssid(ssid);
    simple_wifi_get_password(password);
    ESP_LOGI(TAG, "SoftAP: SSID=%s, password=%s", ssid, password);

    start_settings_httpd(&httpd);

    while (true) {
        action = app_switches_get_action();
        if (action == (APP_ACTION_LEFT|APP_ACTION_FLAG_RELEASE)) {
            break;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    httpd_stop(httpd);
    lan_manager_release_conn();
    return APP_MODE_CLOCK;
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

    ESP_ERROR_CHECK( app_switches_init() );
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
        case APP_MODE_INITIAL: s_mode = handle_initial(); break;
        case APP_MODE_INITIALSYNC: s_mode = handle_initialsync(); break;
        case APP_MODE_CLOCK: s_mode = handle_clock(); break;
        case APP_MODE_SETTINGS: s_mode = handle_settings(); break;
        }
    }
}
