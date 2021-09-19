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

#include <string.h>
#include <gfx.h>
#include <lcd_generic.h>
#include "ssd1306.h"
#include "lcd_ssd1306.h"
#include "ssd1306_common.h"

static inline lcd_ssd1306_t *get_lcd(abstract_lcd_t *this)
{
    return (lcd_ssd1306_t *)this;
}

static inline ssd1306_t *get_device(abstract_lcd_t *this)
{
    return get_lcd(this)->device;
}

static unsigned int lcd_ssd1306_get_width(abstract_lcd_t *this)
{
    return get_device(this)->width;
}
static unsigned int lcd_ssd1306_get_height(abstract_lcd_t *this)
{
    return get_device(this)->height;
}
static void lcd_ssd1306_clear(abstract_lcd_t *this)
{
    ssd1306_t *device = get_device(this);
    memset(device->buffer, 0, device->buffer_size);
}
static void lcd_ssd1306_flush(abstract_lcd_t *this)
{
    ssd1306_flush(get_device(this));
}
static void lcd_ssd1306_set_fg_color(abstract_lcd_t *this, unsigned int color)
{
    get_lcd(this)->fg_color = color != COLOR_BLACK;
}
static void lcd_ssd1306_set_bg_color(abstract_lcd_t *this, unsigned int color)
{
    get_lcd(this)->bg_color = color != COLOR_BLACK;
}
static void lcd_ssd1306_set_drawmode(abstract_lcd_t *this, unsigned int drawmode)
{
    get_lcd(this)->drawmode = drawmode;
}
typedef void (*pixel_func_t)(uint8_t *buffer, int pos, uint8_t b);
static void pixel_noop(uint8_t *buffer, int pos, uint8_t b) {
    (void)buffer; (void)pos; (void)b;
}
static void pixel_set(uint8_t *buffer, int pos, uint8_t b) {
    buffer[pos] |= b;
}
static void pixel_clear(uint8_t *buffer, int pos, uint8_t b) {
    buffer[pos] &= ~b;
}
static void pixel_invert(uint8_t *buffer, int pos, uint8_t b) {
    buffer[pos] ^= b;
}
static pixel_func_t get_pixel_func(unsigned char color, unsigned int drawmode)
{
    if (color) {
        switch (drawmode&3) {
        case DRMODE_COMPLEMENT: return pixel_invert;
        case DRMODE_BG: return pixel_noop;
        case DRMODE_FG: return pixel_set;
        case DRMODE_SOLID: return pixel_set;
        }
    } else {
        switch (drawmode&3) {
        case DRMODE_COMPLEMENT: return pixel_invert;
        case DRMODE_BG: return pixel_noop;
        case DRMODE_FG: return pixel_clear;
        case DRMODE_SOLID: return pixel_clear;
        }
    }
    return pixel_noop;
}

static void lcd_ssd1306_drawpixel(abstract_lcd_t *this, int x, int y)
{
    lcd_ssd1306_t *lcd = get_lcd(this);
    ssd1306_t *device = get_device(this);
    if ((unsigned)x < (unsigned)device->width && (unsigned)y < (unsigned)device->height) {
        int pos = SSD1306_XY2POS(device, x, y);
        uint8_t b = SSD1306_XY2BIT(device, x, y);
        pixel_func_t pixel_func = get_pixel_func(lcd->fg_color, lcd->drawmode);
        if (pixel_func != NULL) {
            pixel_func(device->buffer, pos, b);
        }
    }
}

static abstract_lcd_t base = {
    .get_width = lcd_ssd1306_get_width,
    .get_height = lcd_ssd1306_get_height,
    .clear = lcd_ssd1306_clear,
    .flush = lcd_ssd1306_flush,
    .set_fg_color = lcd_ssd1306_set_fg_color,
    .set_bg_color = lcd_ssd1306_set_bg_color,
    .set_drawmode = lcd_ssd1306_set_drawmode,
    .drawpixel = lcd_ssd1306_drawpixel,
    .hline = lcd_generic_hline,
    .vline = lcd_generic_vline,
    .fillrect = lcd_generic_fillrect_vert,
};

esp_err_t lcd_ssd1306_init(lcd_ssd1306_t *lcd, ssd1306_t *device)
{
    if (lcd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    lcd->base = base;
    lcd->device = device;
    lcd->fg_color = 0;
    lcd->bg_color = 0;
    lcd->drawmode = DRMODE_SOLID;
    return ESP_OK;
}
