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

#include <stdint.h>

#include "gfx_bitmap.h"
#include "lcd.h"
#include "lcd_1bit_vert.h"
#include "trace.h"

void lcd_1bit_vert_hline(gfx_bitmap_t *dst,
    unsigned int color, unsigned int drawmode, int x1, int x2, int y)
{
    const int scansize = dst->header.scansize;
    uint8_t bit = 1<<(y&7);
    uint8_t *row = dst->data + y/8 + x1*scansize;
    if (color) {
        while (x1 <= x2) {
            row[0] |= bit;
            row += scansize;
            x1++;
        }
    } else {
        bit = ~bit;
        while (x1 <= x2) {
            row[0] &= bit;
            row += scansize;
            x1++;
        }
    }
}

void lcd_1bit_vert_vline(gfx_bitmap_t *dst,
    unsigned int color, unsigned int drawmode, int x, int y1, int y2)
{
    const int scansize = dst->header.scansize;
    int row1 = y1/8, row2 = y2/8;
    uint8_t bits = 0xff<<(y1&7);
    uint8_t *col = dst->data + x*scansize;
    if (color) {
        while (row1 < row2) {
            col[row1] |= bits;
            bits = 0xff;
            row1++;
        }
        bits &= 0xff>>((~y2)&7);
        col[row1] |= bits;
    } else {
        bits = ~bits;
        while (row1 < row2) {
            col[row1] &= bits;
            bits = 0x00;
            row1++;
        }
        bits |= 0xfe<<(y2&7);
        col[row1] &= bits;
    }
}

void lcd_1bit_vert_fillrect(gfx_bitmap_t *dst,
    unsigned int color, unsigned int drawmode, int x1, int y1, int x2, int y2)
{
    const int scansize = dst->header.scansize;
    int row1 = y1/8, row2 = y2/8;
    uint8_t bits = 0xff<<(y1&7);
    int x;
    uint8_t *col;
    if (color) {
        while (row1 < row2) {
            col = dst->data + x1*scansize;
            for (x = x1; x <= x2; x++) {
                col[row1] |= bits;
                col += scansize;
            }
            bits = 0xff;
            row1++;
        }
        bits &= 0xff>>((~y2)&7);
        col = dst->data + x1*scansize;
        for (x = x1; x <= x2; x++) {
            col[row1] |= bits;
            col += scansize;
        }
    } else {
        bits = ~bits;
        while (row1 < row2) {
            col = dst->data + x1*scansize;
            for (x = x1; x <= x2; x++) {
                col[row1] &= bits;
                col += scansize;
            }
            bits = 0x00;
            row1++;
        }
        bits |= 0xfe<<(y2&7);
        col = dst->data + x1*scansize;
        for (x = x1; x <= x2; x++) {
            col[row1] &= bits;
            col += scansize;
        }
    }
}

void lcd_1bit_vert_drawbitmap_mono(gfx_bitmap_t *dst,
    const gfx_bitmap_t *src, int src_x, int src_y,
        int x, int y, int width, int height)
{
    const int xstart = x, xend = x+width-1, ystart = y, yend = y+height-1;
    const int srcscansize = src->header.scansize;
    const int dstscansize = dst->header.scansize;
    int sx, sy;
    for (sy = src_y, y = ystart; y <= yend;) {
        const uint8_t *src_row = src->data + sy*srcscansize;
        const int row = y/8;
        const int shift = (y&7);
        uint8_t *dst_pixel = dst->data + row + xstart*dstscansize;
        uint8_t mask, h;
        mask = ((uint8_t)0xff<<(y&shift));
        if (row == yend/8) {
            mask &= ~((uint8_t)0xfe<<(yend&7));
            h = yend - y + 1;
        } else {
            h = 8 - shift;
        }
        for (sx = src_x, x = xstart; x <= xend; sx++, x++) {
            const uint8_t *src_pixel = src_row + sx/8;
            uint8_t srcshift = (sx&7);
            uint8_t srcmask = 0x80>>srcshift;
            uint16_t bits = 0;
            int i;
            for (i = 0; i < h; i++) {
                bits |= (src_pixel[0]&srcmask)<<i;
                src_pixel += srcscansize;
            }
            dst_pixel[0] = (((bits>>(7-srcshift))<<shift)&mask)|(dst_pixel[0]&~(mask));
            dst_pixel += dstscansize;
        }
        sy += h;
        y += h;
    }
}

void lcd_1bit_vert_drawbitmap(gfx_bitmap_t *dst,
    const gfx_bitmap_t *src, int src_x, int src_y,
        int x, int y, int width, int height)
{
    if (src->header.depth == 1) {
        lcd_1bit_vert_drawbitmap_mono(dst, src, src_x, src_y, x, y, width, height);
        return;
    }
    TRACE("lcd 1bit vert: Unsupported depth: %d\n", src->header.depth);
}
