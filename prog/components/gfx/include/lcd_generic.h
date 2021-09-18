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

#include "lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void lcd_generic_vline(abstract_lcd_t *lcd,
    int x, int y1, int y2);
extern void lcd_generic_hline(abstract_lcd_t *lcd,
    int x1, int x2, int y);
extern void lcd_generic_fillrect_vert(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);
extern void lcd_generic_fillrect_horz(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2);

#ifdef __cplusplus
}
#endif
