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
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_task.h>
#include <esp_err.h>
#include <esp_log.h>

#include "simple_wifi.h"
#include "simple_wifi_event.h"
#include "simple_wifi_internal.h"

#define TAG "swifi"

#define STARTED_BIT     BIT0
#define CONNECTED_BIT   BIT1

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

enum simple_wifi_mode simple_wifi_mode;
static SemaphoreHandle_t s_wifi_mutex = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;

ESP_EVENT_DEFINE_BASE(SIMPLE_WIFI_EVENT);

esp_err_t simple_wifi_init(void)
{
    esp_err_t err;

    if (s_wifi_mutex == NULL) {
        s_wifi_mutex = xSemaphoreCreateMutex();
        if (s_wifi_mutex == NULL) {
            return ESP_FAIL;
        }
    }
    err = simple_sta_init();
    if (err != ESP_OK) {
        return err;
    }
    return ESP_OK;
}

bool simple_wifi__lock(TickType_t timeout)
{
    return xSemaphoreTake(s_wifi_mutex, timeout) == pdTRUE;
}

void simple_wifi__unlock(void)
{
    if (xSemaphoreGive(s_wifi_mutex) == pdTRUE) {
        return;
    }
    ESP_ERROR_CHECK( ESP_ERR_INVALID_STATE );
}

esp_err_t simple_wifi_start(enum simple_wifi_mode mode)
{
    if (mode == 0 || mode == SIMPLE_WIFI_MODE_SOFTAP) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!simple_wifi__lock(5000/portTICK_PERIOD_MS)) {
        return ESP_FAIL;
    }
    if (simple_wifi_mode) {
        simple_wifi__unlock();
        return simple_wifi_mode == mode? ESP_OK: ESP_ERR_INVALID_STATE;
    }
    tcpip_adapter_init();
    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
        esp_event_loop_create_default();
    }
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    const wifi_country_t country = {.cc="JP", .schan=1, .nchan=14};
    ESP_ERROR_CHECK( esp_wifi_set_country(&country) );

    if (mode & SIMPLE_WIFI_MODE_SOFTAP) {
        ESP_ERROR_CHECK( simple_softap_configure() );
    }
    if (mode == SIMPLE_WIFI_MODE_STA) {
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    } else if (mode == SIMPLE_WIFI_MODE_SOFTAP) {
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    } else /* SIMPLE_WIFI_MODE_STA_SOFTAP */ {
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );
    }
    ESP_ERROR_CHECK( esp_wifi_start() );
    simple_wifi_mode = mode;
    simple_wifi__unlock();

    EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, STARTED_BIT, pdFALSE, pdTRUE, 3000/portTICK_PERIOD_MS);
    if (! (uxBits & STARTED_BIT) ) {
        simple_wifi_stop();
        return ESP_FAIL;
    }
    return ESP_OK;
}

void simple_wifi_stop(void)
{
    if (simple_wifi_mode & SIMPLE_WIFI_MODE_SOFTAP) {
        simple_softap_stop();
    }
    if (simple_wifi_mode & SIMPLE_WIFI_MODE_STA) {
        simple_sta_stop();
    }
    simple_wifi__lock(portMAX_DELAY);
    simple_wifi_mode = 0;
    simple_wifi__unlock();

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_restore();
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, STARTED_BIT|CONNECTED_BIT);
    }
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler);
}

enum simple_wifi_mode simple_wifi_get_mode(void)
{
    return simple_wifi_mode;
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch(event_id) {
        case WIFI_EVENT_SCAN_DONE:
            simple_sta_set_scan_result();
            break;
        case WIFI_EVENT_STA_START:
            xEventGroupSetBits(s_wifi_event_group, STARTED_BIT);
            break;
        case WIFI_EVENT_STA_STOP:
            simple_wifi__lock(portMAX_DELAY);
            if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
                simple_connection_state = SIMPLE_WIFI_DISCONNECTED;
                swifi_event_post(STA_DISCONNECTED, NULL, 0);
            }
            simple_wifi__unlock();
            xEventGroupClearBits(s_wifi_event_group, STARTED_BIT);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            {
                wifi_event_sta_disconnected_t *event = event_data;
                ESP_LOGV(TAG, "Disconnect reason: %u", event->reason);
            }
            simple_wifi__lock(portMAX_DELAY);
            if (simple_sta_retry == 0 || simple_connection_state != SIMPLE_WIFI_CONNECTED) {
                if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
                    simple_connection_state = SIMPLE_WIFI_DISCONNECTED;
                    swifi_event_post(STA_DISCONNECTED, NULL, 0);
                }
            } else {
                simple_sta_retry--;
                if (xEventGroupGetBits(s_wifi_event_group) & CONNECTED_BIT) {
                    esp_wifi_connect();
                }
                xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            }
            simple_wifi__unlock();
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGD(TAG, "SoftAP start");
            swifi_event_post(SOFTAP_START, NULL, 0);
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGD(TAG, "SoftAP stop");
            swifi_event_post(SOFTAP_STOP, NULL, 0);
            break;
        default:
            ESP_LOGV(TAG, "Unhandled wifi event: %u", event_id);
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch(event_id) {
        case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t *event = event_data;
                ESP_LOGV(TAG, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
                if (simple_connection_state == SIMPLE_WIFI_CONNECTING) {
                    simple_connection_state = SIMPLE_WIFI_CONNECTED;
                }
                swifi_event_post(STA_CONNECTED, NULL, 0);
                simple_sta_retry = 5;
                xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            }
            break;
        default:
            ESP_LOGV(TAG, "Unhandled ip event: %u", event_id);
            break;
        }
    }
}

const char *simple_wifi_auth_mode_to_str(int authmode)
{
    switch ((wifi_auth_mode_t)authmode) {
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA,WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default: return "?";
    }
}

const char *simple_wifi_cipher_type_to_str(int cipher_type)
{
    switch ((wifi_cipher_type_t)cipher_type) {
    case WIFI_CIPHER_TYPE_WEP40: return "WEP";
    case WIFI_CIPHER_TYPE_WEP104: return "WEP";
    case WIFI_CIPHER_TYPE_TKIP: return "TKIP";
    case WIFI_CIPHER_TYPE_CCMP: return "AES";
    case WIFI_CIPHER_TYPE_TKIP_CCMP: return "TKIP/AES";
    default: return "?";
    }
}
