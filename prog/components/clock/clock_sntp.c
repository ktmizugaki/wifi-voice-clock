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

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include <lwip/err.h>
#include <lwip/apps/sntp.h>
#include <sntp/sntp.h>

#include "clock.h"
#include "clock_sync.h"
#include "clock_internal.h"

#define TAG "clock_sntp"

#define CLOCK_SYNC_TIMEOUT  (10*1000/portTICK_PERIOD_MS)

typedef enum {
    CLOCK_SNTP_IDLE,
    CLOCK_SNTP_STARTED,
    CLOCK_SNTP_COMPLETED,
} clock_sntp_state_t;

static TickType_t s_sync_timeout = 0;

static clock_sntp_state_t s_sntp_state = CLOCK_SNTP_IDLE;
static char *s_p_ntp_server = NULL;

static void sync_time_cb(struct timeval *tv)
{
    ESP_LOGD(TAG, "clock synced");
    if (s_sync_timeout != 0) {
        s_sntp_state = CLOCK_SNTP_COMPLETED;
        clock_event_post(CLOCK_EVENT_SYNC_OK, NULL, 0);
    }
}

static esp_err_t clock_start_sntp(void)
{
    if (s_sntp_state != CLOCK_SNTP_IDLE) {
        return ESP_FAIL;
    }
    s_sntp_state = CLOCK_SNTP_STARTED;

    ESP_LOGD(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, s_p_ntp_server);
    sntp_set_time_sync_notification_cb(sync_time_cb);
    sntp_init();
    return ESP_OK;
}

static void clock_stop_sntp(void)
{
    if (s_sntp_state != CLOCK_SNTP_IDLE) {
        sntp_stop();
        sntp_set_time_sync_notification_cb(NULL);
        sntp_setservername(0, NULL);
    }
    s_sntp_state = CLOCK_SNTP_IDLE;
}

esp_err_t clock_sync_sntp_start(const char *ntp_server)
{
    esp_err_t err;

    if (s_sync_timeout != 0) {
        return ESP_OK;
    }
    if (!clock_is_running()) {
        err = clock_start();
        if (err != ESP_OK) {
            return err;
        }
    }

    s_sync_timeout = (xTaskGetTickCount() + CLOCK_SYNC_TIMEOUT) | 1;
    if (s_p_ntp_server != NULL) {
        free(s_p_ntp_server);
    }
    s_p_ntp_server = strdup(ntp_server);
    if (s_p_ntp_server == NULL) {
        return ESP_ERR_NO_MEM;
    }

    return clock_start_sntp();
}

esp_err_t clock_sync_sntp_stop(void)
{
    if (s_sync_timeout == 0) {
        return ESP_OK;
    }
    clock_stop_sntp();
    if (s_p_ntp_server != NULL) {
        free(s_p_ntp_server);
    }
    s_p_ntp_server = NULL;
    s_sync_timeout = 0;
    return ESP_OK;
}

void clock_sync_sntp_process(void)
{
    int remaining;

    if (s_sync_timeout == 0) {
        return;
    }

    remaining = s_sync_timeout - xTaskGetTickCount();
    ESP_LOGV(TAG, "state: %d, %d", s_sntp_state, remaining);

    if (s_sntp_state == CLOCK_SNTP_STARTED && remaining < 0) {
        ESP_LOGV(TAG, "timed out");
        s_sntp_state = CLOCK_SNTP_COMPLETED;
        clock_event_post(CLOCK_EVENT_SYNC_TIMEOUT, NULL, 0);
        clock_sync_sntp_stop();
    }
}
