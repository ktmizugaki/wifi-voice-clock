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
#include <esp_err.h>
#include <esp_log.h>

#include "app_event.h"
#include "app_switches.h"
#include "../misc.h"

#include "app_display.h"
#include "../bitmaps/batt.bmp.h"

#include "liblistview.h"
#include "libmenu.h"

#define TAG "menu"

static inline bool menu_item_exec(const menu_item_t *item, unsigned int *full_update)
{
    if (item->exec.ptr == NULL) {
        return false;
    }
    switch (item->flags & MIF_EXEC_MASK) {
    case MIF_EXEC_FUNC:
        return item->exec.func(item, full_update);
    case MIF_EXEC_SUBMENU:
        return menu_run(item->exec.submenu);
    }
    return false;
}

static const char* menu_item_get_value(const struct menu_item *item,
    char *buf, size_t bufsize, bool full)
{
    switch (item->flags&MIF_VALUE_TYPE_MASK) {
    case MIF_VALUE_TYPE_STR:
        if (full) {
            return item->value.str;
        }
        break;
    case MIF_VALUE_TYPE_FUNC:
        if (item->value.func(item, buf, bufsize, full)) {
            return buf;
        }
        break;
    }
    return NULL;
}

static bool menu_lvitem_exec(listview_item_t *item)
{
    const menu_item_t *menuitem = item->user_data;
    unsigned int full_update;
    char buf[32];
    ESP_LOGD(TAG, "item value: %s", menu_item_get_value(menuitem, buf, sizeof(buf), true));
    return menu_item_exec(menuitem, &full_update);
}

static bool menu_get_lvitem(listview_t *list, uint16_t position, listview_item_t *item)
{
    const menu_t *menu = list->user_data;
    const menu_item_t *menuitem;
    if (position >= menu->item_count) {
        return false;
    }
    menuitem = menu->items[position];
    item->flags = MIF_VALUE_TYPE_STR;
    item->value = menuitem->label;
    item->exec = menu_lvitem_exec;
    item->user_data = (void*)menuitem;
    return true;
}

bool menu_run(const menu_t *menu)
{
    listview_t list = {
        .flags = LVF_ITEMS_TYPE_FUNC,
        .item_count = menu->item_count,
        .title = menu->title,
        .get_item = menu_get_lvitem,
        .user_data = (void*)menu,
    };
    return listview(&list);
}
