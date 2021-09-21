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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct abstract_lcd abstract_lcd_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t scansize;
    uint8_t depth;
} gfx_bitmap_header_t;

typedef struct gfx_bitmap {
    gfx_bitmap_header_t header;
    uint8_t *data;
} gfx_bitmap_t;

extern void gfx_draw_bitmap(abstract_lcd_t *lcd,
    const gfx_bitmap_t *src, int x, int y, int width, int height);

extern void gfx_draw_bitmap_part(abstract_lcd_t *lcd,
    const gfx_bitmap_t *src, int src_x, int src_y,
    int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif
