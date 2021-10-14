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
#include <http_clock_conf.h>
#include <lan_manager.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_mode.h"

#include "app_display.h"
#include "app_display_softap.h"
#include "gen/lang.h"

#define TAG "settings"

static MAKE_EMBEDDED_HANDLER(index_html, "text/html")

esp_err_t http_index_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.method = HTTP_GET;
    http_uri.handler = EMBEDDED_HANDLER_NAME(index_html);
    http_uri.user_ctx = NULL;

    http_uri.uri = "/";
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    http_uri.uri = "/index.html";
    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t http_index_unregister(httpd_handle_t handle)
{
    httpd_unregister_uri_handler(handle, "/", HTTP_GET);
    httpd_unregister_uri_handler(handle, "/index.html", HTTP_GET);
    return ESP_OK;
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
    ESP_ERROR_CHECK( http_index_register(*httpd) );
    ESP_ERROR_CHECK( http_wifi_conf_register(*httpd) );
    ESP_ERROR_CHECK( http_alarm_conf_register(*httpd) );
    ESP_ERROR_CHECK( http_clock_conf_register(*httpd) );
}

app_mode_t app_mode_settings(void)
{
    httpd_handle_t httpd = NULL;
    app_mode_t next_mode = APP_MODE_CLOCK;
    ESP_LOGD(TAG, "handle_settings");

    app_display_ensure_reset();
    app_display_clear();
    gfx_text_puts_xy(LCD, &font_shinonome12, LANG_REMOTE_MAINT, 0, 0);
    gfx_draw_hline(LCD, 0, 12, LCD_WIDTH-1, 12);
    app_display_update();

    if (!lan_manager_request_softap()) {
        ESP_LOGI(TAG, "start softap failed");
        gfx_text_puts_xy(LCD, &gfx_tinyfont, "Start soft AP failed", 0, 0);
        app_display_update();
        return APP_MODE_CLOCK;
    }
    app_display_sfotap(LANG_REMOTE_MAINT);
    app_display_update();

    start_settings_httpd(&httpd);

    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            switch (event.id) {
            case APP_EVENT_ACTION:
                switch (event.arg0) {
                case APP_ACTION_LEFT|APP_ACTION_FLAG_RELEASE:
                    goto end;
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_LONG:
                    next_mode = APP_MODE_SUSPEND;
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
    return next_mode;
}
