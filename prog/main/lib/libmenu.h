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

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct menu menu_t;
typedef struct menu_item menu_item_t;

extern bool menu_run(const menu_t *menu);

struct menu {
    uint16_t item_count;
    const char *title;
    const menu_item_t **items;
};

#define MIF_VALUE_TYPE_MASK     0x01
#define MIF_VALUE_TYPE_STR      0x00
#define MIF_VALUE_TYPE_FUNC     0x01
#define MIF_VALUE_SCROLL_MASK   0x08
#define MIF_VALUE_SCROLL        0x08

#define MIF_EXEC_MASK           0x10
#define MIF_EXEC_FUNC           0x00
#define MIF_EXEC_SUBMENU        0x10

struct menu_item {
    uint8_t flags;
    const char *label;
    union {
        const char *str;
        bool (*func)(const menu_item_t *item,
            char *buf, size_t bufsize, bool full);
    } value;
    union {
        void *ptr;
        bool (*func)(const menu_item_t *item, unsigned int *full_update);
        const menu_t *submenu;
    } exec;
};

#define MAKE_MENU(prefix, name, titlestr, ...) \
    static const menu_item_t *name ## _items[] = {__VA_ARGS__}; \
    prefix const menu_t name = { \
        .item_count = sizeof(name ## _items) / sizeof(menu_item_t *), \
        .title = titlestr, \
        .items = name ## _items, \
    }

#ifdef __cplusplus
}
#endif
