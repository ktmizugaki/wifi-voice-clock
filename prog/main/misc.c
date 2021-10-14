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
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>
#include <clock_conf.h>
#include <alarm.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_mode.h"

#include "misc.h"

#define TAG "misc"

static time_t s_task_check_time = 0;

bool misc_process_time_task(void)
{
    time_t time;
    struct tm tm;
    int range;
    const struct alarm *palarm;
    bool result = false;
    range = (time = clock_time(NULL)) - s_task_check_time;
    if (s_task_check_time == 0 || range > 20 || range < -2) {
        range = 20;
    } else if (range < 0) {
        range = 0;
    }
    s_task_check_time = time;
    clock_localtime(&tm);
    if (clock_conf_is_sync_time(&tm, range)) {
        app_clock_start_sync();
        result = true;
    }
    if (alarm_get_current_alarm(&tm, range, &palarm)) {
        // TODO: play_alarm(palarm);
        //result = true;
    }
    return result;
}

void misc_handle_event(const app_event_t *event)
{
    switch (event->id) {
    case APP_EVENT_CLOCK:
        misc_process_time_task();
        break;
    case APP_EVENT_SYNC:
        if (event->arg0 == APP_SYNC_FAIL || event->arg0 == APP_SYNC_SUCCESS) {
            app_clock_stop_sync();
        }
        break;
    default:
        break;
    }
}
