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
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <simple_wifi.h>
#include <simple_wifi_event.h>
#include <http_wifi_conf.h>

#include "lan_manager.h"

#define TAG "lanm"

#define SYNCHRONIZE_BIT BIT0
#define CONNECTED_BIT   BIT1

static bool s_connecting = false;
static int16_t s_ref_count = 0;
static EventGroupHandle_t s_event_group = NULL;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == SIMPLE_WIFI_EVENT) {
        switch(event_id) {
        case SIMPLE_WIFI_EVENT_STA_CONNECTED:
            if (s_connecting) {
                xEventGroupSetBits(s_event_group, CONNECTED_BIT);
            }
            break;
        }
    }
}

static esp_err_t start_softap(void)
{
    esp_err_t err;

    if (!http_wifi_conf_configured()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (simple_wifi_get_mode() == 0) {
        err = simple_wifi_start(SIMPLE_WIFI_MODE_STA_SOFTAP);
    } else if ((simple_wifi_get_mode() & SIMPLE_WIFI_MODE_SOFTAP) == 0) {
        /* wifi was started with incompatible mode */
        err = ESP_ERR_INVALID_STATE;
    } else {
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "wifi is busy");
        return err;
    }

    return ESP_OK;
}

static esp_err_t start_connect(void)
{
    esp_err_t err;

    if (!http_wifi_conf_configured()) {
        return ESP_ERR_INVALID_STATE;
    }

    if (simple_wifi_get_mode() == 0) {
        err = simple_wifi_start(SIMPLE_WIFI_MODE_STA);
    } else if ((simple_wifi_get_mode() & SIMPLE_WIFI_MODE_STA) == 0) {
        /* wifi was started with incompatible mode */
        err = ESP_ERR_INVALID_STATE;
    } else {
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "wifi is busy");
        return err;
    }

    ESP_ERROR_CHECK( esp_event_handler_register(
        SIMPLE_WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
    err = http_wifi_conf_connect();
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "start connecting failed");
        return err;
    }

    return ESP_OK;
}

esp_err_t lan_manager_init(void)
{
    if (s_event_group == NULL) {
        s_event_group = xEventGroupCreate();
        if (s_event_group == NULL) {
            return ESP_ERR_NO_MEM;
        }
        xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);
    }
    return ESP_OK;
}

bool lan_manager_request_softap(void)
{
    esp_err_t err;

    xEventGroupWaitBits(s_event_group, pdTRUE, pdTRUE, SYNCHRONIZE_BIT, portMAX_DELAY);

    s_ref_count++;

    err = start_softap();
    if (err != ESP_OK) {
        s_ref_count--;
        xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);
        return false;
    }
    xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);

    return true;
}

bool lan_manager_request_conn(void)
{
    esp_err_t err;

    xEventGroupWaitBits(s_event_group, pdTRUE, pdTRUE, SYNCHRONIZE_BIT, portMAX_DELAY);

    s_ref_count++;

    if (!s_connecting) {
        err = start_connect();
        if (err != ESP_OK) {
            s_ref_count--;
            xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);
            return false;
        }
        s_connecting = true;
    }
    xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);

    return true;
}

void lan_manager_release_conn(void)
{
    xEventGroupWaitBits(s_event_group, pdTRUE, pdTRUE, SYNCHRONIZE_BIT, portMAX_DELAY);

    if (--s_ref_count == 0) {
        simple_wifi_stop();
        s_connecting = false;
    } else if (s_ref_count < 0) {
        ESP_LOGE(TAG, "ref count underflow");
        abort();
    }

    xEventGroupSetBits(s_event_group, SYNCHRONIZE_BIT);
}

bool lan_manager_wait_conn(int timeout_ms)
{
    TickType_t waitTick = timeout_ms < 0? portMAX_DELAY:
        (timeout_ms+portTICK_PERIOD_MS-1)/portTICK_PERIOD_MS;
    EventBits_t uxBits;

    uxBits = xEventGroupWaitBits(s_event_group, CONNECTED_BIT,
        pdFALSE, pdFALSE, waitTick);
    return (uxBits & CONNECTED_BIT) != 0;
}
