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

#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <gfx.h>
#include <lcd.h>
#include <ssd1306.h>
#include <lcd_ssd1306.h>

#include "app_display.h"

#define TAG "display"

static const ssd1306_param_t s_param = {
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT,
    .gpio = {
        .mosi = GPIO_NUM_23,
        .sclk = GPIO_NUM_18,
        .cs = GPIO_NUM_5,
    },
    .dc = GPIO_NUM_22,
    .reset = GPIO_NUM_19,
};

static lcd_ssd1306_t s_lcd = {};
static ssd1306_t s_device = {};
static bool s_reset = false;
static bool s_on = false;

esp_err_t app_display_init(void)
{
    esp_err_t err;

    if (s_lcd.device != NULL) {
        return ESP_OK;
    }
    err = ssd1306_init_gpio(&s_device, &s_param);
    if (err != ESP_OK) {
        ssd1306_deinit(&s_device);
        return ESP_FAIL;
    }
    err = lcd_ssd1306_init(&s_lcd, &s_device);
    if (err != ESP_OK) {
        ssd1306_deinit(&s_device);
        return ESP_FAIL;
    }
    return ESP_OK;
}

abstract_lcd_t* app_display_get(void)
{
    if (s_lcd.device == NULL) {
        return NULL;
    }
    return &s_lcd.base;
}

void app_display_ensure_reset(void)
{
    if (!s_reset) {
        s_reset= true;
        app_display_reset();
    }
}

void app_display_reset(void)
{
    s_on = true;
    ssd1306_reset(&s_device);
    app_display_clear();
}

void app_display_on(void)
{
    if (!s_on) {
        s_on = true;
        ssd1306_on(&s_device);
    }
}

void app_display_off(void)
{
    if (s_on) {
        s_on = false;
        ssd1306_sleep(&s_device);
    }
}

void app_display_clear(void)
{
    memset(s_device.buffer, 0, s_device.buffer_size);
}

void app_display_update(void)
{
    ssd1306_flush(&s_device);
    app_display_on();
}
