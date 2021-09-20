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
#include <math.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>

#include "app_clock.h"
#include "app_display.h"
#include "app_display_clock.h"

#define TAG "main_clock"

typedef struct {
    short x;
    short y;
} point_t;

static void rotate(int deg, int r, int l, point_t *pt1, point_t *pt2)
{
    double rad = (deg-90)*M_PI/180;
    double c = cos(rad), s = sin(rad);
    pt2->x = round(0.5 + (l+0.25) * c - 0.5*s);
    pt2->y = round(0.5 + (l+0.25) * s + 0.5*c);
    pt1->x = round(0.5 + (r+0.25) * c - 0.5*s);
    pt1->y = round(0.5 + (r+0.25) * s + 0.5*c);
}

static void draw_hand(int x, int y, int deg, int r, int l, int width)
{
    point_t pt1, pt2;

    rotate(deg, r, l, &pt1, &pt2);
    gfx_draw_thick_line(LCD, x+pt1.x, y+pt1.y, x+pt2.x, y+pt2.y, width);
}

void app_display_clock(const struct tm *tm)
{
    const point_t POS = { 32, 0 };
    const int RADIUS = 32;
    const point_t CENTER = { POS.x + RADIUS-1, POS.y + RADIUS-1 };

    gfx_set_fg_color(LCD, COLOR_BLACK);
    gfx_fill_ellipse(LCD, POS.x, POS.y, POS.x+RADIUS*2-1, POS.y+RADIUS*2-1);
    gfx_set_fg_color(LCD, COLOR_WHITE);
    gfx_draw_ellipse(LCD, POS.x, POS.y, POS.x+RADIUS*2-1, POS.y+RADIUS*2-1);

    draw_hand(CENTER.x, CENTER.y, (tm->tm_hour % 12) * 30 + (tm->tm_min * 30 / 60), 4, RADIUS*4/7, 2);
    draw_hand(CENTER.x, CENTER.y, tm->tm_min * 6 + (tm->tm_sec * 6 / 60), 4, RADIUS*5/6, 2);
    draw_hand(CENTER.x, CENTER.y, tm->tm_sec * 6, 4, RADIUS-3, 1);
}
