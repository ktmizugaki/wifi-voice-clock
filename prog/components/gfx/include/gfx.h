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

#ifdef __cplusplus
extern "C" {
#endif

#define COLOR_BLACK     0x000000
#define COLOR_WHITE     0xffffff
#define DRMODE_COMPLEMENT 0
#define DRMODE_BG         1
#define DRMODE_FG         2
#define DRMODE_SOLID      3

typedef struct abstract_lcd abstract_lcd_t;

extern unsigned int gfx_get_width(abstract_lcd_t *lcd);
extern unsigned int gfx_get_height(abstract_lcd_t *lcd);
extern void gfx_clear(abstract_lcd_t *lcd);
extern void gfx_flush(abstract_lcd_t *lcd);

extern void gfx_set_fg_color(abstract_lcd_t *lcd, unsigned int color);
extern void gfx_set_bg_color(abstract_lcd_t *lcd, unsigned int color);
extern void gfx_set_drawmode(abstract_lcd_t *lcd, unsigned int drawmode);

#include "gfx_primitive.h"

#ifdef __cplusplus
}
#endif
