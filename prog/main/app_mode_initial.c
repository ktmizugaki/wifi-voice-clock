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
#include <wifi_conf.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_mode.h"

#include "app_display.h"
#include "app_display_softap.h"
#include "gen/lang.h"

#define TAG "initial"

static esp_err_t http_get_fallback_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Location", "/wifi_conf");
    return httpd_resp_sendstr(req, "");
}

static esp_err_t http_fallback_register(httpd_handle_t handle)
{
    esp_err_t err;

    static httpd_uri_t http_uri;
    http_uri.method = HTTP_GET;
    http_uri.uri = "/*";
    http_uri.handler = http_get_fallback_handler;
    http_uri.user_ctx = NULL;

    err = httpd_register_uri_handler(handle, &http_uri);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

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
    ESP_ERROR_CHECK( http_fallback_register(*httpd) );
}

app_mode_t app_mode_initial(void)
{
    httpd_handle_t httpd = NULL;
    ESP_LOGD(TAG, "handle_initial");

    app_display_ensure_reset();
    app_display_clear();
    gfx_text_puts_xy(LCD, &font_shinonome12, LANG_INITIAL_CONF, 0, 0);
    gfx_draw_hline(LCD, 0, 12, LCD_WIDTH-1, 12);
    app_display_update();

    simple_wifi_clear_ap();
    simple_wifi_start(SIMPLE_WIFI_MODE_STA_SOFTAP);

    app_display_sfotap(LANG_INITIAL_CONF);
    app_display_update();

    start_initial_httpd(&httpd);

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
            default:
                break;
            }
        }
    }

end:
    httpd_stop(httpd);
    return wifi_conf_configured()? APP_MODE_INITIALSYNC: APP_MODE_INITIAL;
}
