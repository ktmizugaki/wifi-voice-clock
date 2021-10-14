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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_switches.h"
#include "misc.h"
#include "app_mode.h"

#include "app_display.h"
#include "app_display_clock.h"
#include "gen/lang.h"

#define TAG "main_clock"

struct clock_mode_state {
    int8_t idle;
    int8_t on;
    int8_t date_on;
};

static void update_clock(struct clock_mode_state *state)
{
    struct tm tm;
    char buf_date[11], buf_time[11];
    int w, h;
    clock_localtime(&tm);
    strftime(buf_date, sizeof(buf_date), "%m/%d %a", &tm);
    strftime(buf_time, sizeof(buf_time), "%H:%M:%S", &tm);
    printf("Time is: %s %s\n", buf_date, buf_time);
    if (state->date_on >= 0) {
        gfx_set_fg_color(LCD, COLOR_BLACK);
        gfx_text_get_bounds(LCD, &font_shinonome14, LANG_DIGITS, NULL, NULL, NULL, &h);
        gfx_fill_rect(LCD, 2, LCD_HEIGHT-h, LCD_WIDTH-2, LCD_HEIGHT);
    }
    app_display_clock(&tm);
    if (state->date_on > 0) {
        strftime(buf_date, sizeof(buf_date), "%Y.", &tm);
        strftime(buf_time, sizeof(buf_time), "%m.%d", &tm);
        gfx_set_fg_color(LCD, COLOR_WHITE);
        gfx_text_get_bounds(LCD, &font_shinonome14, buf_date, NULL, NULL, &w, &h);
        gfx_text_puts_xy(LCD, &font_shinonome14, buf_date, 2, LCD_HEIGHT-h);

        gfx_text_get_bounds(LCD, &font_shinonome14, buf_time, NULL, NULL, &w, &h);
        gfx_text_puts_xy(LCD, &font_shinonome14, buf_time, LCD_WIDTH-2-w, LCD_HEIGHT-h);
    }
    app_display_update();
}

app_mode_t app_mode_clock(void)
{
    struct clock_mode_state state = { 0, 15, 8, };
    ESP_LOGD(TAG, "handle_clock");
    ESP_ERROR_CHECK( app_display_ensure_init() );

    app_display_ensure_reset();
    app_display_clear();
    update_clock(&state);

    while (true) {
        app_event_t event;
        if (app_event_get(&event)) {
            misc_handle_event(&event);
            switch (event.id) {
            case APP_EVENT_ACTION:
                state.idle = 0;
                switch (event.arg0) {
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_LONG:
                    return APP_MODE_SUSPEND;
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_RELEASE:
                    return APP_MODE_SETTINGS;
                default:
                    if (state.on <= 0) {
                        state.on = 20;
                        state.date_on = 8;
                        app_display_on();
                        update_clock(&state);
                    } else {
                        state.on = 20;
                    }
                    break;
                }
                break;
            case APP_EVENT_CLOCK:
                state.idle++;
                if (state.idle >= 20) {
                    return APP_MODE_SUSPEND;
                }
                if (state.on > 0) {
                    state.on--;
                    if (state.on == 0) {
                        app_display_off();
                    } else {
                        if (state.date_on >= 0) {
                            state.date_on--;
                        }
                        update_clock(&state);
                    }
                }
                break;
            default:
                break;
            }
        }
    }

    return APP_MODE_CLOCK;
}
