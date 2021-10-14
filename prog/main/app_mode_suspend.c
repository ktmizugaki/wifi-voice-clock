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

#include <clock.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_display.h"
#include "power.h"
#include "misc.h"
#include "app_mode.h"

#define TAG "suspend"

static bool is_all_task_done(void)
{
    if (!app_clock_is_done()) {
        return false;
    }
    return true;
}

app_mode_t app_mode_suspend(void)
{
    int to = 10;
    ESP_LOGD(TAG, "handle_suspend");
    app_display_off();

    if (is_all_task_done()) {
        goto suspend;
    }
    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            misc_handle_event(&event);
            switch (event.id) {
            case APP_EVENT_ACTION:
                return APP_MODE_CLOCK;
            case APP_EVENT_CLOCK:
                if (misc_process_time_task()) {
                    if (to < 10) {
                        to = 10;
                    }
                } else if (to > 0) {
                    if (--to == 0) {
                        goto suspend;
                    }
                }
                break;
            default:
                break;
            }
            if (is_all_task_done()) {
                goto suspend;
            }
        }
    }

suspend:
    power_suspend();
}
