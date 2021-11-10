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

/**
 * @file
 * clock of this system.
 * send event at each second to do time based tasks,
 * e.g. check whether work is timed out or not.
 */

/** event base of clock */
ESP_EVENT_DECLARE_BASE(CLOCK);

/** clock events */
typedef enum {
    CLOCK_EVENT_SECOND,         /** sent at each second. */
    CLOCK_EVENT_SYNC_OK,        /** clock finished to synchronize to source. */
    CLOCK_EVENT_SYNC_FAIL,      /** clock failed to synchronize to source. */
    CLOCK_EVENT_SYNC_TIMEOUT,   /** clock synchronization operation timed out. */
} clock_event_t;

/**
 * @brief get time of the clock.
 * returned value is rounded value obtained from <pre>gettimeofday</pre>.
 * because of FreeRTOS's vTaskDelay accuracy, the task that waiting the time to
 * send CLOCK_EVENT_SECOND event cannot wake at exact real second,
 * i.e. it may wake at few milliseconds before exact real second.
 * instead of try to be accurate, disguise current time for efficiency.
 */
extern time_t clock_time(time_t *t);
/** @brief helper function to get localtime of current clock time. */
extern struct tm *clock_localtime(struct tm *tm);

/** @brief start sending clock events. */
extern esp_err_t clock_start(void);
/** @brief stop sending clock events. */
extern void clock_stop(void);
/**
 * @brief register an event handler to receive clock events.
 * see esp_event document of ESP-IDF about event handlers.
 * @note clock does not use system default event loop.
 * @param[in] event_handler the event handler function.
 * @param[in] arg           this value is passed to event_handler_arg of the event_handler.
 * @return ESP_OK for success, other value for failure.
 */
extern esp_err_t clock_register_event_handler(esp_event_handler_t event_handler, void *arg);
/**
 * @brief unregister an event handler with clock events.
 * @param[in] event_handler the event handler function.
 */
extern void clock_unregister_event_handler(esp_event_handler_t event_handler);

/** @brief return true if clock event task is running. */
extern bool clock_is_running(void);
/** @brief return true if clock points at sane time. */
extern bool clock_is_valid(void);

#ifdef __cplusplus
}
#endif
