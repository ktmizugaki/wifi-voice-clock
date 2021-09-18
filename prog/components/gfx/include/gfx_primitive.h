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

extern void gfx_draw_pixel(abstract_lcd_t *lcd,
    int x, int y);

extern void gfx_draw_vline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);

extern void gfx_draw_hline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);

extern void gfx_draw_line(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);

extern void gfx_draw_rect(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);
extern void gfx_fill_rect(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);
extern void gfx_draw_triangle(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3);
extern void gfx_fill_triangle(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3);
extern void gfx_draw_ellipse(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);
extern void gfx_fill_ellipse(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);

extern void gfx_draw_thick_line(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int thickness);

#ifdef __cplusplus
}
#endif
