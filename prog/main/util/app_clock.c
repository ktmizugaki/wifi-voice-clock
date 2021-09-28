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
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <simple_wifi.h>
#include <simple_wifi_event.h>
#include <wifi_conf.h>
#include <lan_manager.h>
#include <clock.h>
#include <clock_sync.h>

#include "app_clock.h"
#include "app_event.h"

#define TAG "clock"

enum sync_state_t {
    SYNC_STATE_NONE,
    SYNC_STATE_WAITING_CONNECTED,
    SYNC_STATE_WAITING_SNTP,
    SYNC_STATE_DONE,
};

static bool s_syncing = false;
static enum sync_state_t s_sync_state = SYNC_STATE_NONE;

static void start_sntp(void)
{
    char ntp_server[WIFI_CONF_NTP_SERVER_LENGTH] = "";
    esp_err_t err;

    err = wifi_conf_get_ntp(ntp_server);
    if (err != ESP_OK || ntp_server[0] == '\0') {
        s_sync_state = SYNC_STATE_DONE;
        app_event_send_arg(APP_EVENT_SYNC, APP_SYNC_FAIL);
        return;
    }
    ESP_LOGI(TAG, "start sntp to %s", ntp_server);
    clock_sync_sntp_start(ntp_server);
    s_sync_state = SYNC_STATE_WAITING_SNTP;
    app_event_send_arg(APP_EVENT_SYNC, APP_SYNC_SNTP);
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == SIMPLE_WIFI_EVENT) {
        switch(event_id) {
        case SIMPLE_WIFI_EVENT_STA_CONNECTED:
            if (s_sync_state == SYNC_STATE_WAITING_CONNECTED) {
                start_sntp();
            }
            break;
        case SIMPLE_WIFI_EVENT_STA_DISCONNECTED:
        case SIMPLE_WIFI_EVENT_STA_FAIL:
            if (s_sync_state == SYNC_STATE_WAITING_CONNECTED) {
                s_sync_state = SYNC_STATE_DONE;
                app_event_send_arg(APP_EVENT_SYNC, APP_SYNC_FAIL);
            }
            break;
        }
    } else if (event_base == CLOCK) {
        switch(event_id) {
        case CLOCK_EVENT_SECOND:
            app_event_send(APP_EVENT_CLOCK);
            break;
        case CLOCK_EVENT_SYNC_OK:
        case CLOCK_EVENT_SYNC_FAIL:
        case CLOCK_EVENT_SYNC_TIMEOUT:
            if (s_sync_state == SYNC_STATE_WAITING_SNTP) {
                if (event_id == CLOCK_EVENT_SYNC_OK) {
                    ESP_LOGI(TAG, "clock successfully synced");
                } else {
                    ESP_LOGI(TAG, "clock failed to sync");
                }
                s_sync_state = SYNC_STATE_DONE;
                app_event_send_arg(APP_EVENT_SYNC,
                    event_id==CLOCK_EVENT_SYNC_OK? APP_SYNC_SUCCESS: APP_SYNC_FAIL);
            }
            break;
        }
    }
}

esp_err_t app_clock_init(void)
{
    ESP_ERROR_CHECK( esp_event_handler_register(
        SIMPLE_WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
    ESP_ERROR_CHECK( clock_register_event_handler(event_handler, NULL) );
    ESP_ERROR_CHECK( clock_start() );
    return ESP_OK;
}

esp_err_t app_clock_start_sync(void)
{
    if (s_syncing) {
        ESP_LOGD(TAG, "already syncing");
        return ESP_OK;
    }

    if (!lan_manager_request_conn()) {
        s_sync_state = SYNC_STATE_NONE;
        ESP_LOGI(TAG, "start connecting failed");
        return ESP_ERR_INVALID_STATE;
    }

    s_sync_state = SYNC_STATE_WAITING_CONNECTED;
    s_syncing = true;

    if (lan_manager_is_connected()) {
        start_sntp();
    }
    return ESP_OK;
}

void app_clock_stop_sync(void)
{
    if (s_syncing) {
        s_sync_state = SYNC_STATE_NONE;
        lan_manager_release_conn();
        s_syncing = false;
    }
}

bool app_clock_is_done(void)
{
    return s_sync_state == SYNC_STATE_NONE || s_sync_state == SYNC_STATE_DONE;
}
