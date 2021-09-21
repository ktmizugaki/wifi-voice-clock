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

typedef struct abstract_lcd abstract_lcd_t;
typedef struct gfx_bitmap gfx_bitmap_t;

struct abstract_lcd {
    unsigned int (*get_width)(abstract_lcd_t *this);
    unsigned int (*get_height)(abstract_lcd_t *this);
    void (*clear)(abstract_lcd_t *this);
    void (*flush)(abstract_lcd_t *this);

    void (*set_fg_color)(abstract_lcd_t *this, unsigned int color);
    void (*set_bg_color)(abstract_lcd_t *this, unsigned int color);
    void (*set_drawmode)(abstract_lcd_t *this, unsigned int drawmode);

    void (*drawpixel)(abstract_lcd_t *this, int x, int y);
    void (*vline)(abstract_lcd_t *this, int x, int y1, int y2);
    void (*hline)(abstract_lcd_t *this, int x1, int x2, int y);
    void (*fillrect)(abstract_lcd_t *this, int x1, int y1, int x2, int y2);
    void (*drawbitmap)(abstract_lcd_t *this,
        const gfx_bitmap_t *src, int src_x, int src_y,
        int x, int y, int width, int height);
};

#ifdef __cplusplus
}
#endif
