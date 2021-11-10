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
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <esp_event.h>

#include "clock.h"
#include "clock_internal.h"

#define TAG "clock"

#define CLOCK_THRESHOLD_MS  10

ESP_EVENT_DEFINE_BASE(CLOCK);

struct clock_state {
    bool running;
    esp_event_loop_handle_t loop;
    TaskHandle_t task_handle;
};

static struct clock_state s_clock_state = {
    .running = false,
    .loop = NULL,
    .task_handle = NULL,
};

static void clock_task(void*arg)
{
    time_t curr_sec;
    clock_time(&curr_sec);
    while (1) {
        struct timeval tv;
        int d;
        gettimeofday(&tv, NULL);

        while (1) {
            if (curr_sec == tv.tv_sec+1) {
                d = 2000 - tv.tv_usec / 1000 + portTICK_PERIOD_MS/3;
            } else if (curr_sec == tv.tv_sec) {
                d = 1000 - tv.tv_usec / 1000 + portTICK_PERIOD_MS/3;
                if (d < CLOCK_THRESHOLD_MS) {
                    curr_sec = tv.tv_sec+1;
                    break;
                }
            } else {
                /* time jumped */
                curr_sec = tv.tv_sec;
                break;
            }
            vTaskDelay(d / portTICK_PERIOD_MS);
            gettimeofday(&tv, NULL);
        }
        clock_event_post(CLOCK_EVENT_SECOND, &curr_sec, sizeof(curr_sec));
        esp_event_loop_run(s_clock_state.loop, 0);
        clock_sync_sntp_process();
        esp_event_loop_run(s_clock_state.loop, 300/portTICK_PERIOD_MS);
    }
}

void clock_event_post(clock_event_t event, void *data, size_t data_size)
{
    if (s_clock_state.loop != NULL) {
        esp_event_post_to(s_clock_state.loop, CLOCK, event,
            data, data_size, 10);
    }
}

time_t clock_time(time_t *t)
{
    time_t res;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    res = tv.tv_sec + (tv.tv_usec + CLOCK_THRESHOLD_MS*1000LU) / 1000000LU;
    if (t != NULL) {
        *t = res;
    }
    return res;
}

struct tm *clock_localtime(struct tm *tm)
{
    time_t t;
    clock_time(&t);
    return localtime_r(&t, tm);
}

esp_err_t clock_init(void)
{
    esp_err_t err;
    /* events are processed in clock_task */
    esp_event_loop_args_t loop_args = {
        .queue_size = 6,
        .task_name = NULL,
        .task_stack_size = 0,
        .task_priority = 0,
        .task_core_id = 0,
    };

    if (s_clock_state.loop != NULL) {
        return ESP_OK;
    }

    err = esp_event_loop_create(&loop_args, &s_clock_state.loop);
    if (err != ESP_OK) {
        s_clock_state.loop = NULL;
    }
    return err;
}

esp_err_t clock_start(void)
{
    BaseType_t ret;
    esp_err_t err;

    if (s_clock_state.running) {
        ESP_LOGD(TAG, "clock is already started");
        return ESP_OK;
    }

    err = clock_init();
    if (err != ESP_OK) {
        return err;
    }
    ret = xTaskCreate(clock_task, "clock_task", 16*1024, NULL, 5,
        &s_clock_state.task_handle);
    if (ret != pdPASS) {
        return ESP_FAIL;
    }
    s_clock_state.running = true;
    return ESP_OK;
}

void clock_stop(void)
{
    if (!s_clock_state.running) {
        return;
    }
    if (s_clock_state.task_handle != NULL) {
        vTaskDelete(s_clock_state.task_handle);
        s_clock_state.task_handle = NULL;
    }
    s_clock_state.running = false;
}

esp_err_t clock_register_event_handler(esp_event_handler_t event_handler, void *arg)
{
    esp_err_t err;

    err = clock_init();
    if (err != ESP_OK) {
        return err;
    }
    return esp_event_handler_register_with(s_clock_state.loop,
        CLOCK, ESP_EVENT_ANY_ID, event_handler, arg);
}

void clock_unregister_event_handler(esp_event_handler_t event_handler)
{
    if (s_clock_state.loop == NULL) {
        return;
    }
    esp_event_handler_unregister_with(s_clock_state.loop,
        CLOCK, ESP_EVENT_ANY_ID, event_handler);
}

bool clock_is_running(void)
{
    return s_clock_state.running;
}

bool clock_is_valid(void)
{
    time_t now = 0;
    struct tm tm = { 0 };
    time(&now);
    localtime_r(&now, &tm);
    return tm.tm_year >= (2000 - 1900);
}
