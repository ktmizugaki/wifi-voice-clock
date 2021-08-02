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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <esp_err.h>

#include <esp_event.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(CLOCK);

typedef enum {
    CLOCK_EVENT_SECOND,
    CLOCK_EVENT_SYNC_OK,
    CLOCK_EVENT_SYNC_FAIL,
    CLOCK_EVENT_SYNC_TIMEOUT,
} clock_event_t;

#define CLOCK_THRESHOLD_MS  10

/* Get time with rounded seconds.
 * This is needed because CLOCK_EVENT_SECOND event may be sent a few milliseconds
 * before exact second. */
extern time_t clock_time(time_t *t);
extern struct tm *clock_localtime(struct tm *tm);

extern esp_err_t clock_start(void);
extern void clock_stop(void);
extern esp_err_t clock_register_event_handler(esp_event_handler_t event_handler, void *arg);
extern void clock_unregister_event_handler(esp_event_handler_t event_handler);

extern bool clock_is_running(void);
extern bool clock_is_valid(void);

#ifdef __cplusplus
}
#endif
