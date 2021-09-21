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

#define FONT_MAGIC_STR  "KT01"
#define FONT_MAGIC  { 0x4B, 0x54, 0x30, 0x31 }

typedef struct {
    uint16_t unicode;
    uint16_t bitmap_offset;
    uint8_t width;
    uint8_t x_advance;
    int8_t x_offset;
    int8_t y_offset;
} gfx_glyph_t;

typedef struct {
    uint8_t magic[4];
    uint16_t glyph_count;
    uint16_t first;
    uint16_t last;
    uint16_t default_char;
    uint8_t height;
    gfx_glyph_t *glyphs;
    uint8_t *bitmap;
} gfx_font_t;

extern void gfx_text_puts_xy(abstract_lcd_t *lcd,
    const gfx_font_t *font, const char *str, int x, int y);
extern void gfx_text_get_bounds(abstract_lcd_t *lcd,
    const gfx_font_t *font, const char *str,
    int *x, int *y, int *width, int *height);
extern const gfx_glyph_t* gfx_text_get_glyph(abstract_lcd_t *lcd,
    const gfx_font_t *font, uint16_t unicode);

#ifdef __cplusplus
}
#endif
