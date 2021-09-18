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
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <esp_log.h>

#include "app_event.h"

#define TAG "event"

static QueueHandle_t s_event_queue = NULL;

esp_err_t app_event_init(void)
{
    if (s_event_queue == NULL) {
        s_event_queue = xQueueCreate(12, sizeof(app_event_t));
        if (s_event_queue == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

void app_event_send(app_event_id_t event_id)
{
    app_event_send_args(event_id, 0, 0);
}

void app_event_send_arg(app_event_id_t event_id, int arg)
{
    app_event_send_args(event_id, arg, 0);
}

void app_event_send_args(app_event_id_t event_id, int arg0, int arg1)
{
    app_event_t event = { event_id, arg0, arg1 };
    if (s_event_queue != NULL) {
        xQueueSend(s_event_queue, &event, 0);
    }
}

bool app_event_get(app_event_t *event)
{
    return app_event_get_to(event, -1);
}

bool app_event_get_to(app_event_t *event, int timeout_ms)
{
    if (s_event_queue != NULL) {
        TickType_t timeout = timeout_ms<0? portMAX_DELAY: timeout_ms/portTICK_PERIOD_MS;
        return xQueueReceive(s_event_queue, event, timeout) != 0;
    }
    return false;
}
