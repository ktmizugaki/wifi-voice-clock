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
#include <gfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH   128
#define LCD_HEIGHT  64
#define LCD         app_display_get()

extern esp_err_t app_display_init(void);
extern abstract_lcd_t* app_display_get(void);
extern void app_display_ensure_reset(void);
extern void app_display_reset(void);
extern void app_display_on(void);
extern void app_display_off(void);
extern void app_display_clear(void);
extern void app_display_update(void);

#ifdef __cplusplus
}
#endif
