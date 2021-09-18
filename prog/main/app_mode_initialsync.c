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

#define TAG "initialsync"

static void update_clock(void)
{
    struct tm tm;
    char buf_date[11], buf_time[11];
    clock_localtime(&tm);
    strftime(buf_date, sizeof(buf_date), "%m/%d %a", &tm);
    strftime(buf_time, sizeof(buf_time), "%H:%M:%S", &tm);
    printf("Time is: %s %s\n", buf_date, buf_time);
}

app_mode_t app_mode_initialsync(void)
{
    esp_err_t err;
    ESP_LOGD(TAG, "handle_initialsync");

    err = app_clock_start_sync();
    if (err != ESP_OK) {
        return APP_MODE_INITIAL;
    }

    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            switch (event.id) {
            case APP_EVENT_ACTION:
                break;
            case APP_EVENT_CLOCK:
                update_clock();
                break;
            case APP_EVENT_SYNC:
                if (app_clock_is_done()) {
                    goto end;
                }
            }
        }
    }

end:
    app_clock_stop_sync();
    return clock_is_valid()? APP_MODE_CLOCK: APP_MODE_INITIAL;
}
