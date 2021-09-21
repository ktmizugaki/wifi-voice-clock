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

#include "lcd.h"
#include "gfx_bitmap.h"
#include "trace.h"

void gfx_draw_bitmap(abstract_lcd_t *lcd,
    const gfx_bitmap_t *src, int x, int y, int width, int height)
{
    gfx_draw_bitmap_part(lcd, src, 0, 0, x, y, width, height);
}

void gfx_draw_bitmap_part(abstract_lcd_t *lcd,
    const gfx_bitmap_t *src, int src_x, int src_y,
    int x, int y, int width, int height)
{
    const int dst_width = (int)lcd->get_width(lcd);
    const int dst_height = (int)lcd->get_height(lcd);

    /* crop values */
    if (x < 0) {
        src_x += -x;
        width -= -x;
        x = 0;
    }
    if (y < 0) {
        src_y += -y;
        height -= -y;
        y = 0;
    }
    if (width > src->header.width - src_x) {
        width = src->header.width - src_x;
    }
    if (height > src->header.height - src_y) {
        height = src->header.height - src_y;
    }
    if (width > dst_width - x) {
        width = dst_width - x ;
    }
    if (height > dst_height - y) {
        height = dst_height - y;
    }
    /* if x exceeds dst_width, width is negative and if y exceeds dst_height,
     * height is negative, so no need to test x and y against dst_width and dst_height */
    if (width <= 0 || height <= 0) {
        return;
    }

    lcd->drawbitmap(lcd, src, src_x, src_y, x, y, width, height);
}
