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
#include "gfx.h"
#include "trace.h"

unsigned int gfx_get_width(abstract_lcd_t *lcd)
{
    return lcd->get_width(lcd);
}

unsigned int gfx_get_height(abstract_lcd_t *lcd)
{
    return lcd->get_height(lcd);
}

void gfx_clear(abstract_lcd_t *lcd)
{
    lcd->clear(lcd);
}

void gfx_flush(abstract_lcd_t *lcd)
{
    lcd->flush(lcd);
}

void gfx_set_fg_color(abstract_lcd_t *lcd, unsigned int color)
{
    lcd->set_fg_color(lcd, color);
}

void gfx_set_bg_color(abstract_lcd_t *lcd, unsigned int color)
{
    lcd->set_bg_color(lcd, color);
}

void gfx_set_drawmode(abstract_lcd_t *lcd, unsigned int drawmode)
{
    lcd->set_drawmode(lcd, drawmode);
}
