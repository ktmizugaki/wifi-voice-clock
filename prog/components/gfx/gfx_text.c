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

#include <stdlib.h>
#include <stdint.h>

#include "lcd.h"
#include "gfx_text.h"
#include "gfx_bitmap.h"
#include "trace.h"

static uint16_t get_ucs2(const char **str)
{
    int n;
    uint16_t ucs;
    uint8_t c = *(*str);
    if (c == 0) {
        return 0;
    }
    (*str)++;
    if (c < 0x80) {
        return c;
    }
    if (c < 0xc0) {
        return 0xFFFD;
    } else if (c < 0xe0) {
        c &= 0x1f;
        n = 1;
    } else if (c < 0xf0) {
        c &= 0x0f;
        n = 2;
    } else {
        return 0xFFFD;
    }
    ucs = c;
    while (n > 0) {
        c = *(*str);
        if (c == 0) {
            return 0xFFFD;
        }
        (*str)++;
        if (c < 0x80 || c >= 0xc0) {
            return 0xFFFD;
        }
        ucs = (ucs<<6)|(c&0x3f);
        n--;
    }
    return ucs;
}

void gfx_text_puts_xy(abstract_lcd_t *lcd,
    const gfx_font_t *font, const char *str, int x, int y)
{
    const int width = (int)lcd->get_width(lcd);
    while (*str && x < width) {
        uint16_t unicode = get_ucs2(&str);
        const gfx_glyph_t *glyph = gfx_text_get_glyph(lcd, font, unicode);
        gfx_bitmap_t src;
        if (glyph == NULL) {
            glyph = gfx_text_get_glyph(lcd, font, font->default_char);
        }
        src.header.width = glyph->width;
        src.header.height = font->height;
        src.header.scansize = (glyph->width+7)/8;
        src.header.depth = 1;
        src.data = font->bitmap+glyph->bitmap_offset;
        gfx_draw_bitmap(lcd, &src, x+glyph->x_offset, y+glyph->y_offset,
            src.header.width, src.header.height);

        x += glyph->x_advance;
    }
}

void gfx_text_get_bounds(abstract_lcd_t *lcd,
    const gfx_font_t *font, const char *str,
    int *x, int *y, int *width, int *height)
{
    int x0 = 0, y0 = 0, w = 0, h = font->height;
    int dx = 0, dy = 0;
    while (*str) {
        uint16_t unicode = get_ucs2(&str);
        const gfx_glyph_t *glyph = gfx_text_get_glyph(lcd, font, unicode);
        if (glyph == NULL) {
            glyph = gfx_text_get_glyph(lcd, font, font->default_char);
        }
        if (x0 > w+glyph->x_offset) {
            x0 = w+glyph->x_offset;
        }
        if (dx < glyph->x_offset+glyph->width) {
            dx = glyph->x_offset+glyph->width;
        }
        w += glyph->x_advance;
        dx -= glyph->x_advance;

        if (y0 > glyph->y_offset) {
            y0 = glyph->y_offset;
        }
        if (dy < glyph->y_offset) {
            dy = glyph->y_offset;
        }
    }
    if (dx > 0) {
        w += dx;
    }
    if (dy > 0) {
        h += dy;
    }
    if (x != NULL) *x = x0;
    if (y != NULL) *y = y0;
    if (width != NULL) *width = w;
    if (height != NULL) *height = h;
}

const gfx_glyph_t* gfx_text_get_glyph(abstract_lcd_t *lcd,
    const gfx_font_t *font, uint16_t unicode)
{
    uint16_t top, bottom;
    if (unicode < font->first || unicode > font->last) {
        return NULL;
    }
    top = 0;
    bottom = font->glyph_count-1;
    while (top <= bottom) {
        uint16_t range = bottom-top+1;
        uint16_t index = (bottom+top)/2;
        const gfx_glyph_t *glyph;
        index = unicode - font->glyphs[top].unicode;
        if (index < range) {
            glyph = &font->glyphs[index];
            if (glyph->unicode == unicode) {
                return glyph;
            }
        }
        index = (bottom+top)/2;
        glyph = &font->glyphs[index];
        if (glyph->unicode == unicode) {
            return glyph;
        } else if (glyph->unicode > unicode) {
            bottom = index-1;
        } else {
            top = index+1;
        }
    }
    return NULL;
}
