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
#include <esp_err.h>
#include <esp_log.h>

#include <simple_wifi.h>

#include "app_display.h"
#include "app_display_softap.h"
#include "gen/lang.h"

#define TAG "main_softap"

void app_display_sfotap(const char *title)
{
    char ssid[SWIFI_SSID_LEN];
    char password[SWIFI_PW_LEN];
    int w, x, y;

    simple_wifi_get_ssid(ssid);
    simple_wifi_get_password(password);
    ESP_LOGI(TAG, "SoftAP: SSID=%s, password=%s", ssid, password);

    gfx_text_puts_xy(LCD, &font_shinonome12, title, 0, 0);
    gfx_draw_hline(LCD, 0, 12, LCD_WIDTH-1, 12);
    gfx_text_puts_xy(LCD, &font_shinonome12, LANG_SSID, 0, 14);
    gfx_text_get_bounds(LCD, &gfx_tinyfont, ssid, NULL, NULL, &w, NULL);
    if (6+w < LCD_WIDTH-4) {
        x = 6;
        y = 26;
    } else {
        x = LCD_WIDTH-4-w;
        y = 26;
    }
    gfx_text_puts_xy(LCD, &gfx_tinyfont, ssid, x, y);
    y += 8;
    gfx_text_puts_xy(LCD, &font_shinonome12, LANG_PASSWORD, 0, y);
    y += 12;
    gfx_text_puts_xy(LCD, &gfx_tinyfont, password, 6, y);
}
