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

#include <esp_err.h>
#include <lcd.h>
#include "ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    abstract_lcd_t base;
    ssd1306_t *device;
    unsigned char fg_color;
    unsigned char bg_color;
    unsigned int drawmode;
} lcd_ssd1306_t;

extern esp_err_t lcd_ssd1306_init(lcd_ssd1306_t *lcd, ssd1306_t *device);

#ifdef __cplusplus
}
#endif
