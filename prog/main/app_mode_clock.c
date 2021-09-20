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
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "app_mode.h"

#include "app_display.h"
#include "app_display_clock.h"

#define TAG "main_clock"

static void update_clock(void)
{
    struct tm tm;
    char buf_date[11], buf_time[11];
    clock_localtime(&tm);
    strftime(buf_date, sizeof(buf_date), "%m/%d %a", &tm);
    strftime(buf_time, sizeof(buf_time), "%H:%M:%S", &tm);
    printf("Time is: %s %s\n", buf_date, buf_time);
    app_display_clock(&tm);
    app_display_update();
}

app_mode_t app_mode_clock(void)
{
    bool clock_on = true;
    ESP_LOGD(TAG, "handle_clock");
    app_display_ensure_reset();
    app_display_clear();
    update_clock();

    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            switch (event.id) {
            case APP_EVENT_ACTION:
                switch (event.arg0) {
                case APP_ACTION_LEFT|APP_ACTION_FLAG_RELEASE:
                    if (clock_on) {
                        ESP_LOGD(TAG, "turn off clock");
                        clock_on = false;
                        app_display_off();
                    }
                    break;
                case APP_ACTION_RIGHT|APP_ACTION_FLAG_RELEASE:
                    if (!clock_on) {
                        ESP_LOGD(TAG, "turn on clock");
                        clock_on = true;
                        app_display_on();
                        update_clock();
                    }
                    break;
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_RELEASE:
                    return APP_MODE_SETTINGS;
                }
                break;
            case APP_EVENT_CLOCK:
                if (clock_on) {
                    update_clock();
                }
                break;
            case APP_EVENT_SYNC:
                break;
            }
        }
    }

    return APP_MODE_CLOCK;
}
