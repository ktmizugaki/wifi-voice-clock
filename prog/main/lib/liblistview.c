/* Copyright 2022 Kawashima Teruaki
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
#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>
#include <vcc.h>

#include "app_event.h"
#include "app_switches.h"
#include "../misc.h"

#include "app_display.h"
#include "../bitmaps/batt.bmp.h"

#include "liblistview.h"

#define TAG "listview"

#define LISTVIEW_HEADER_HEIGHT  13
#define LISTVIEW_FOOTER_HEIGHT  13
#define LISTVIEW_CONTENT_HEIGHT (LCD_HEIGHT - LISTVIEW_HEADER_HEIGHT - LISTVIEW_FOOTER_HEIGHT)

typedef struct listview_state_t {
    uint16_t top;
    uint16_t bottom;
    uint16_t left;
    int max_width;
    int current;
} listview_state_t;

static void draw_title(const listview_t *list)
{
    gfx_text_puts_xy(LCD, &font_shinonome12, list->title, 0, 0);
    gfx_draw_hline(LCD, 0, 12, LCD_WIDTH-1, 12);
}

static void draw_batt_time(const listview_t *list)
{
    struct tm tm;
    vcc_charge_state_t state;
    char buf_time[11];
    int w, h;
    int index;

    clock_localtime(&tm);
    strftime(buf_time, sizeof(buf_time), "%H:%M", &tm);

    gfx_set_fg_color(LCD, COLOR_BLACK);
    gfx_fill_rect(LCD, 0, LCD_HEIGHT-13, BATT_BMP_SUBWIDTH, LCD_HEIGHT-1);

    index = (int)vcc_get_level(false);
    if (vcc_get_charge_state(&state) == ESP_OK && state == VCC_CHARG_CHARGING) {
        index = 5;
    }
    if (index >= 0 && index < BATT_BMP_SUBIMG) {
        gfx_draw_bitmap_part(LCD, &batt_bmp, 0, BATT_BMP_SUBHEIGHT*index,
            0, LCD_HEIGHT-12, BATT_BMP_SUBWIDTH, BATT_BMP_SUBHEIGHT);
    }

    gfx_text_get_bounds(LCD, &font_shinonome12, buf_time, NULL, NULL, &w, &h);
    gfx_text_puts_xy(LCD, &font_shinonome12, buf_time, LCD_WIDTH-2-w, LCD_HEIGHT-h);
}

static const listview_item_t *get_item(const listview_t *list, uint16_t position, listview_item_t *storage)
{
    if (position >= list->item_count) {
        ESP_LOGI(TAG, "position exceeds item_count %d/%d", position, list->item_count);
        return NULL;
    }
    switch (list->flags&LVF_ITEMS_TYPE_MASK) {
    case LVF_ITEMS_TYPE_ARRAY:
        if (list->items == NULL) {
            ESP_LOGI(TAG, "items array is not set");
            return NULL;
        }
        return &list->items[position];
    case LVF_ITEMS_TYPE_FUNC:
        if (list->get_item == NULL) {
            ESP_LOGI(TAG, "item func is not set");
            return NULL;
        }
        if (!list->get_item((listview_t *)list, position, storage)) {
            ESP_LOGI(TAG, "get_item returned false");
            return NULL;
        }
        return storage;
    default:
        return NULL;
    }
}

static bool get_item_value(const listview_item_t *item, char *buf, size_t bufsize, bool full)
{
    switch (item->flags&LVIF_VALUE_TYPE_MASK) {
    case LVIF_VALUE_TYPE_STR:
        if (item->value == NULL) {
            return false;
        }
        strlcpy(buf, item->value, bufsize);
        return true;
    case LVIF_VALUE_TYPE_FUNC:
        if (!item->get_value((listview_item_t*)item, buf, bufsize, full)) {
            return false;
        }
        return true;
    default:
        return false;
    }
}

static void draw_listitems(const listview_t *list, listview_state_t *state)
{
    listview_item_t itemstorage;
    const listview_item_t *item = &itemstorage;
    int x, y;
    int w, h;
    int content_height = LISTVIEW_CONTENT_HEIGHT;
    uint16_t item_index;
    char buf[128];
    x = -(int)state->left;
    y = LISTVIEW_HEADER_HEIGHT;
    gfx_set_fg_color(LCD, COLOR_BLACK);
    gfx_fill_rect(LCD, 0, y, LCD_WIDTH-1, y+content_height-1);
    for (item_index = state->top; item_index < list->item_count; item_index++) {
        item = get_item(list, item_index, &itemstorage);
        if (item == NULL) {
            ESP_LOGD(TAG, "Failed to get item at %d", item_index);
            break;
        }
        if (!get_item_value(item, buf, sizeof(buf), true)) {
            ESP_LOGD(TAG, "Failed to get item value at %d", item_index);
            break;
        }
        gfx_text_get_bounds(LCD, &font_shinonome12, buf, NULL, NULL, &w, &h);
        if (w > state->max_width) {
            state->max_width = w;
        }
        if (h > content_height) {
            break;
        }
        gfx_set_fg_color(LCD, COLOR_WHITE);
        gfx_text_puts_xy(LCD, &font_shinonome12, buf, x+2, y);
        if (item_index == state->current) {
            gfx_draw_hline(LCD, 0, y+h-1, LCD_WIDTH-1, y+h-1);
        }
        y += h;
        content_height -= h;
    }
    state->bottom = item_index;
}

static bool exec_item(const listview_t *list, int position)
{
    listview_item_t itemstorage;
    const listview_item_t *item = &itemstorage;
    item = get_item(list, position, &itemstorage);
    if (item == NULL || item->exec == NULL) {
        return false;
    }
    return item->exec((void*)item);
}

static void listview_incr(const listview_t *list, listview_state_t *state)
{
    uint16_t top = state->top, bottom = state->bottom;
    int current = state->current;
    current++;
    if (current >= (int)list->item_count) {
        current = 0;
    }
    if (top > 0 && current-1 < (int)top) {
        top = current-1>0? current-1: 0;
    }
    if (bottom < list->item_count && current+1 >= (int)bottom) {
        top++;
    }
    state->bottom += top-state->top;
    state->top = top;
    state->current = current;
}

static bool listview_hscroll(const listview_t *list, listview_state_t *state, int delta)
{
    uint16_t left = state->left;
    int width = LCD_WIDTH;
    if (delta == 0) {
        return false;
    }
    if (delta < 0 && left == 0) {
        return false;
    }
    if (delta > 0 && (state->max_width < width || left == state->max_width - width)) {
        return false;
    }
    if ((int)left+delta < 0 || state->max_width < width) {
        left = 0;
    } else if ((int)left+delta > state->max_width - width) {
        left = state->max_width - width;
    } else {
        left += delta;
    }
    state->left = left;
    return true;
}

bool listview(const listview_t *list)
{
    listview_state_t state = {0, 0, 0, 0, 0};
    int idle = 0;
    int repeat = 0;
    TickType_t rep_next;

    if (list == NULL || list->item_count == 0) {
        return false;
    }

    app_display_clear();
    draw_title(list);
    draw_batt_time(list);
    draw_listitems(list, &state);
    app_display_update();

    while (true) {
        app_event_t event;
        if (app_event_get_to(&event, repeat? (rep_next-xTaskGetTickCount())*portTICK_PERIOD_MS: -1)) {
            misc_handle_event(&event);
            switch (event.id) {
            case APP_EVENT_ACTION:
                idle = 0;
                if (event.arg0 & APP_ACTION_FLAG_RELEASE) {
                    repeat = 0;
                    rep_next = 0;
                }
                switch (event.arg0) {
                case APP_ACTION_LEFT|APP_ACTION_FLAG_RELEASE:
                    return false;
                    break;
                case APP_ACTION_RIGHT|APP_ACTION_FLAG_RELEASE:
                    if (exec_item(list, state.current)) {
                        return true;
                    }
                    app_display_clear();
                    draw_title(list);
                    draw_batt_time(list);
                    draw_listitems(list, &state);
                    app_display_update();
                    break;
                case APP_ACTION_LEFT|APP_ACTION_FLAG_LONG:
                    if (listview_hscroll(list, &state, -4)) {
                        draw_listitems(list, &state);
                        app_display_update();
                        repeat = event.arg0;
                        rep_next = xTaskGetTickCount() + 400/portTICK_PERIOD_MS;
                    }
                    break;
                case APP_ACTION_RIGHT|APP_ACTION_FLAG_LONG:
                    if (listview_hscroll(list, &state, 4)) {
                        draw_listitems(list, &state);
                        app_display_update();
                        repeat = event.arg0;
                        rep_next = xTaskGetTickCount() + 400/portTICK_PERIOD_MS;
                    }
                    break;
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_PRESS:
                    listview_incr(list, &state);
                    draw_listitems(list, &state);
                    app_display_update();
                    break;
                case APP_ACTION_MIDDLE|APP_ACTION_FLAG_LONG:
                    repeat = event.arg0;
                    rep_next = xTaskGetTickCount() + 200/portTICK_PERIOD_MS;
                    break;
                default:
                    break;
                }
                break;
            case APP_EVENT_CLOCK:
                idle++;
                if (idle >= 30) {
                    return true;
                }
                draw_batt_time(list);
                break;
            default:
                break;
            }
        } else {
            rep_next = xTaskGetTickCount() + 200/portTICK_PERIOD_MS;
            switch (repeat) {
            case APP_ACTION_MIDDLE|APP_ACTION_FLAG_LONG:
                listview_incr(list, &state);
                draw_listitems(list, &state);
                app_display_update();
                break;
            case APP_ACTION_LEFT|APP_ACTION_FLAG_LONG:
                if (listview_hscroll(list, &state, -4)) {
                    draw_listitems(list, &state);
                    app_display_update();
                }
                break;
            case APP_ACTION_RIGHT|APP_ACTION_FLAG_LONG:
                if (listview_hscroll(list, &state, 4)) {
                    draw_listitems(list, &state);
                    app_display_update();
                }
                break;
            }
        }
    }
}
