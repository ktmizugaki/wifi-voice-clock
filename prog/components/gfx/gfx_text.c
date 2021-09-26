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
#include <string.h>

#include "lcd.h"
#include "gfx_text.h"
#include "gfx_bitmap.h"
#include "trace.h"

#if 0
static char *ucs22str(uint16_t ucs, char str[4])
{
    if (ucs < 0x80) {
        str[0] = ucs;
        str[1] = 0;
    } else if (ucs < 0x0800) {
        str[0] = 0xc0|(ucs>>6);
        str[1] = 0x80|(ucs&0x3f);
        str[2] = 0;
    } else {
        str[0] = 0xe0|(ucs>>12);
        str[1] = 0x80|((ucs>>6)&0x3f);
        str[2] = 0x80|(ucs&0x3f);
        str[3] = 0;
    }
    return str;
}
#endif

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

bool gfx_font_from_mem(gfx_font_t *font, const void *memory, size_t size)
{
    const uint8_t *ptr = memory;
    int i;
    uint16_t unicode;
    if (size < 16) {
        /* header is too small */
        return false;
    }
    memcpy(font->magic, ptr, 4); ptr += 4;
    memcpy(&font->glyph_count, ptr, 2); ptr += 2;
    memcpy(&font->first, ptr, 2); ptr += 2;
    memcpy(&font->last, ptr, 2); ptr += 2;
    memcpy(&font->default_char, ptr, 2); ptr += 2;
    memcpy(&font->height, ptr, 1); ptr += 1;
    ptr += 3;
    size -= 16;
    if (memcmp(font->magic, FONT_MAGIC_STR, 4) != 0) {
        /* not font magic */
        return false;
    }
    if (font->glyph_count > size/8) {
        /* glyph_count is too large */
        return false;
    }
    if (font->first > font->last || font->first+font->glyph_count > font->last) {
        /* invalid first or last */
        return false;
    }
    if (font->default_char < font->first || font->default_char > font->last) {
        /* invalid default_char */
        return false;
    }
    if (font->height == 0) {
        /* invalid height */
        return false;
    }
    font->glyphs = (gfx_glyph_t*)ptr;
    font->bitmap = ptr+font->glyph_count*8;
    size -= font->glyph_count*8;
    unicode = 0;
    for (i = 0; i < font->glyph_count; i++) {
        const gfx_glyph_t *glyph = font->glyphs+i;
        int offset = glyph->bitmap_offset;
        offset += ((glyph->width+7)/8) & font->height;
        if (offset >= size) {
            /* invalid bitmap offset in glyph at %d */
            return false;
        }
        if (i > 0 && !(unicode < glyph->unicode)) {
            /* unicode is not strictly increasing */
            return false;
        }
        unicode = glyph->unicode;
    }
    return true;
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
        src.data = (uint8_t*)font->bitmap+glyph->bitmap_offset;
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
