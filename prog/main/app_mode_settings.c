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
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_err.h>
#include <esp_log.h>

#include <simple_wifi.h>
#include <http_html_cmn.h>
#include <http_wifi_conf.h>
#include <http_alarm_conf.h>
#include <lan_manager.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_mode.h"

#define TAG "settings"

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

app_mode_t app_mode_settings(void)
{
    httpd_handle_t httpd = NULL;
    char ssid[SWIFI_SSID_LEN];
    char password[SWIFI_PW_LEN];
    ESP_LOGD(TAG, "handle_settings");

    if (!lan_manager_request_softap()) {
        ESP_LOGI(TAG, "start softap failed");
        return APP_MODE_CLOCK;
    }

    simple_wifi_get_ssid(ssid);
    simple_wifi_get_password(password);
    ESP_LOGI(TAG, "SoftAP: SSID=%s, password=%s", ssid, password);

    start_settings_httpd(&httpd);

    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            switch (event.id) {
            case APP_EVENT_ACTION:
                switch (event.arg0) {
                case APP_ACTION_LEFT|APP_ACTION_FLAG_RELEASE:
                    goto end;
                }
                break;
            case APP_EVENT_CLOCK:
                break;
            case APP_EVENT_SYNC:
                break;
            }
        }
    }

end:
    httpd_stop(httpd);
    lan_manager_release_conn();
    return APP_MODE_CLOCK;
}
