/* Copyright (c) 2013 Adafruit Industries.  All rights reserved.
 * Copyright (c) 2016 Alois Zingl
 * Copyright 2021 Kawashima Teruaki
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

#include <stdlib.h>
#include <math.h>

#include "lcd.h"
#include "gfx_primitive.h"
#include "trace.h"

/* Implementations are imported from forrowing pages/libraries.
 * - The Beauty of Bresenham's Algorithm
 *   http://members.chello.at/easyfilter/bresenham.html
 * - Adafruit-GFX library
 *   https://github.com/adafruit/Adafruit-GFX-Library
 */

#define SWAP(a, b, tmp) { tmp = b; b = a; a = tmp; }
#define REORDER(a, b, tmp) if (a > b) SWAP(a, b, tmp)
#define CAP(a, b, min, max) { \
  if (a < min) a = min; \
  if (b > max) b = max; \
}

void gfx_draw_pixel(abstract_lcd_t *lcd, int x, int y)
{
    const int width = (int)lcd->get_width(lcd);
    const int height = (int)lcd->get_height(lcd);

    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }

    lcd->drawpixel(lcd, x, y);
}

void gfx_draw_vline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    const int height = (int)lcd->get_height(lcd);
    const int width = (int)lcd->get_width(lcd);
    int tmp;
    (void)x2;

    if (x1 < 0 || x1 >= width) {
        return;
    }

    REORDER(y1, y2, tmp);
    CAP(y1, y2, 0, height-1);
    if (y1 > y2) {
        return;
    }

    lcd->vline(lcd, x1, y1, y2);
}

void gfx_draw_hline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    const int width = (int)lcd->get_width(lcd);
    const int height = (int)lcd->get_height(lcd);
    int tmp;
    (void)y2;

    if (y1 < 0 || y1 >= height) {
        return;
    }

    REORDER(x1, x2, tmp);
    CAP(x1, x2, 0, width-1);
    if (x1 > x2) {
        return;
    }

    lcd->hline(lcd, x1, x2, y1);
}

void gfx_draw_line(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    int tmp, steep;
    int dx, dy, err, step, a, b, da, db;

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx == 0 && dy == 0) {
        TRACE("line: is dot: %d, %d\n", x1, y1);
        gfx_draw_pixel(lcd, x1, y1);
        return;
    }
    if (dx == 0) {
        TRACE("line: is vline: %d, %d, %d\n", x1, y1, y2);
        gfx_draw_vline(lcd, x1, y1, x2, y2);
        return;
    }
    if (dy == 0) {
        TRACE("line: is hline: %d, %d, %d\n", x1, x2, y1);
        gfx_draw_hline(lcd, x1, y1, x2, y2);
        return;
    }

    steep = abs(dy) > abs(dx);
    if (steep) {
        da = dy;
        db = dx;
    } else {
        da = dx;
        db = dy;
    }
    if (da < 0) {
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
        da = -da;
        db = -db;
    }

    err = da / 2;
    if (db >= 0) {
        step = 1;
    } else {
        db = -db;
        step = -1;
    }

    for (a = 0, b = 0; a <= da; a++) {
        if (steep) {
            gfx_draw_pixel(lcd, x1+b, y1+a);
        } else {
            gfx_draw_pixel(lcd, x1+a, y1+b);
        }
        err -= db;
        if (err < 0) {
            b += step;
            err += da;
        }
    }
}

void gfx_draw_rect(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    const int width = (int)lcd->get_width(lcd);
    const int height = (int)lcd->get_height(lcd);
    int tmp;

    if (x1 == x2 || y1 == y2) {
        TRACE("rect: is line: %d, %d, %d, %d\n", x1, y1, x2, y2);
        gfx_draw_line(lcd, x1, y1, x2, y2);
        return;
    }

    REORDER(x1, x2, tmp);
    REORDER(y1, y2, tmp);
    if (x1 >= width || x2 < 0 || y1 >= height || y2 < 0) {
        /* nothing to draw */
        TRACE("rect: nothing to draw: %d, %d, %d, %d, %dx%d\n",
            x1, y1, x2, y2, width, height);
        return;
    }

    gfx_draw_vline(lcd, x1, y1, x1, y2);
    gfx_draw_vline(lcd, x2, y1, x2, y2);
    x1++;
    x2--;
    gfx_draw_hline(lcd, x1, y1, x2, y1);
    gfx_draw_hline(lcd, x1, y2, x2, y2);
}

void gfx_fill_rect(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    const int width = (int)lcd->get_width(lcd);
    const int height = (int)lcd->get_height(lcd);
    int tmp;

    if (x1 == x2 || y1 == y2) {
        TRACE("rect: is line: %d, %d, %d, %d\n", x1, y1, x2, y2);
        gfx_draw_line(lcd, x1, y1, x2, y2);
        return;
    }

    REORDER(x1, x2, tmp);
    REORDER(y1, y2, tmp);
    if (x1 >= width || x2 < 0 || y1 >= height || y2 < 0) {
        /* nothing to draw */
        TRACE("rect: nothing to draw: %d, %d, %d, %d, %dx%d\n",
            x1, y1, x2, y2, width, height);
        return;
    }

    CAP(x1, x2, 0, width-1);
    CAP(y1, y2, 0, height-1);

    lcd->fillrect(lcd, x1, y1, x2, y2);
}

void gfx_draw_triangle(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3)
{
    gfx_draw_line(lcd, x1, y1, x2, y2);
    gfx_draw_line(lcd, x2, y2, x3, y3);
    gfx_draw_line(lcd, x3, y3, x1, y1);
}

static void gfx_fill_triangle_vline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3)
{
    const int width = (int)lcd->get_width(lcd);
    int tmp;
    int dx12, dy12, dx13, dy13, dx23, dy23;
    int sa, sb, a, b, x, last;

    if (x1 > x2) {
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }
    if (x2 > x3) {
        SWAP(x2, x3, tmp);
        SWAP(y2, y3, tmp);
    }
    if (x1 > x2) {
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }
    if (x1 >= width || x3 < 0) {
        /* nothing to draw */
        return;
    }

    dx12 = x2 - x1;
    dy12 = y2 - y1;
    dx13 = x3 - x1;
    dy13 = y3 - y1;
    dx23 = x3 - x2;
    dy23 = y3 - y2;

    if (x2 == x3) {
        last = x2;
    } else {
        last = x2 - 1;
    }
    if (last >= width) {
        last = width - 1;
    }
    sa = 0;
    sb = 0;
    for (x = x1; x <= last; x++) {
        a = y1 + sa / dx12; /* y1 + (y2 - y1) * (x - x1) / (x2 - x1) */
        b = y1 + sb / dx13; /* y1 + (y3 - y1) * (x - x1) / (x3 - x1) */
        sa += dy12;
        sb += dy13;
        TRACE("triangle phase1: %d, %d-%d\n", x, a, b);
        gfx_draw_vline(lcd, x, a, x, b);
    }

    last = x3;
    if (last >= width) {
        last = width - 1;
    }
    sa = dy23 * (x - x2);
    sb = dy13 * (x - x1);
    for (; x <= last; x++) {
        a = y2 + sa / dx23; /* y2 + (y3 - y2) * (x - x2) / (x3 - x2) */
        b = y1 + sb / dx13; /* y1 + (y3 - y1) * (x - x1) / (x3 - x1) */
        sa += dy23;
        sb += dy13;
        TRACE("triangle phase2: %d, %d-%d\n", x, a, b);
        gfx_draw_vline(lcd, x, a, x, b);
    }
}

static void gfx_fill_triangle_hline(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3)
{
    const int height = (int)lcd->get_height(lcd);
    int tmp;
    int dx12, dy12, dx13, dy13, dx23, dy23;
    int sa, sb, a, b, y, last;

    if (y1 > y2) {
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }
    if (y2 > y3) {
        SWAP(x2, x3, tmp);
        SWAP(y2, y3, tmp);
    }
    if (y1 > y2) {
        SWAP(x1, x2, tmp);
        SWAP(y1, y2, tmp);
    }
    if (y1 >= height || y3 < 0) {
        /* nothing to draw */
        return;
    }

    dx12 = x2 - x1;
    dy12 = y2 - y1;
    dx13 = x3 - x1;
    dy13 = y3 - y1;
    dx23 = x3 - x2;
    dy23 = y3 - y2;

    if (y2 == y3) {
        last = y2;
    } else {
        last = y2 - 1;
    }
    if (last >= height) {
        last = height - 1;
    }
    sa = 0;
    sb = 0;
    for (y = y1; y <= last; y++) {
        a = x1 + sa / dy12; /* x1 + (x2 - x1) * (y - y1) / (y2 - y1) */
        b = x1 + sb / dy13; /* x1 + (x3 - x1) * (y - y1) / (y3 - y1) */
        sa += dx12;
        sb += dx13;
        TRACE("triangle phase1: %d-%d, %d\n", a, b, y);
        gfx_draw_hline(lcd, a, y, b, y);
    }

    last = y3;
    if (last >= height) {
        last = height - 1;
    }
    sa = dx23 * (y - y2);
    sb = dx13 * (y - y1);
    for (; y <= last; y++) {
        a = x2 + sa / dy23; /* x2 + (x3 - x2) * (y - y2) / (y3 - y2) */
        b = x1 + sb / dy13; /* x1 + (x3 - x1) * (y - y1) / (y3 - y1) */
        sa += dx23;
        sb += dx13;
        TRACE("triangle phase2: %d-%d, %d\n", a, b, y);
        gfx_draw_hline(lcd, a, y, b, y);
    }
}

void gfx_fill_triangle(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int x3, int y3)
{
    int tmp;
    if (x1 == x2 && y1 == y2) {
        gfx_draw_line(lcd, x1, y1, x3, y3);
        return;
    }
    if (x2 == x3 && y2 == y3) {
        gfx_draw_line(lcd, x2, y2, x1, y1);
        return;
    }
    if (x3 == x1 && y3 == y1) {
        gfx_draw_line(lcd, x3, y3, x2, y2);
        return;
    }
    if (x1 == x2 && x2 == x3) {
        REORDER(y1, y2, tmp);
        REORDER(y2, y3, tmp);
        REORDER(y1, y2, tmp);
        gfx_draw_vline(lcd, x1, y1, x3, y3);
        return;
    }
    if (y1 == y2 && y2 == y3) {
        REORDER(x1, x2, tmp);
        REORDER(x2, x3, tmp);
        REORDER(x1, x2, tmp);
        gfx_draw_hline(lcd, x1, y1, x3, y3);
        return;
    }

    gfx_fill_triangle_vline(lcd, x1, y1, x2, y2, x3, y3);
    (void)gfx_fill_triangle_hline;
}

void gfx_draw_ellipse(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    int tmp;
    int a, b, b1; /* values of diameter */
    long dx, dy; /* error increment */
    long err, e2; /* error of 1.step */

    REORDER(x1, x2, tmp);
    REORDER(y1, y2, tmp);
    a = x2 - x1;
    b = y2 - y1;
    b1 = b&1;

    dx = 4*(1-a)*b*b;
    dy = 4*(b1+1)*a*a;
    err = dx+dy+b1*a*a;

    /* starting pixel */
    y1 += (b+1)/2;
    y2 = y1-b1;
    a *= 8*a;
    b1 = 8*b*b;

    TRACE("ellipse: %d-%d, %d-%d; %d, %d, %d, %ld, %ld, %ld\n",
        x1, x2, y2, y1, a, b, b1, dx, dy, err);
    do {
        gfx_draw_pixel(lcd, x2, y1); /*   I. Quadrant */
        gfx_draw_pixel(lcd, x1, y1); /*  II. Quadrant */
        gfx_draw_pixel(lcd, x1, y2); /* III. Quadrant */
        gfx_draw_pixel(lcd, x2, y2); /*  IV. Quadrant */
        e2 = 2*err;
        if (e2 <= dy) { /* y step */
            y1++;
            y2--;
            dy += a;
            err += dy;
        }
        if (e2 >= dx || 2*err > dy) { /* x step */
            x1++;
            x2--;
            dx += b1;
            err += dx;
        }
    } while (x1 <= x2);

    while (y1-y2 < b) {  /* too early stop of flat ellipses a=1 */
        gfx_draw_pixel(lcd, x1-1, y1); /* -> finish tip of ellipse */
        gfx_draw_pixel(lcd, x2+1, y1++);
        gfx_draw_pixel(lcd, x1-1, y2);
        gfx_draw_pixel(lcd, x2+1, y2--);
    }
}

void gfx_fill_ellipse(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2)
{
    int tmp;
    int a, b, b1; /* values of diameter */
    long dx, dy; /* error increment */
    long err, e2; /* error of 1.step */

    REORDER(x1, x2, tmp);
    REORDER(y1, y2, tmp);
    a = x2 - x1;
    b = y2 - y1;
    b1 = b&1;

    dx = 4*(1-a)*b*b;
    dy = 4*(b1+1)*a*a;
    err = dx+dy+b1*a*a;

    /* starting pixel */
    y1 += (b+1)/2;
    y2 = y1-b1;
    a *= 8*a;
    b1 = 8*b*b;

    TRACE("ellipse: %d-%d, %d-%d; %d, %d, %d, %ld, %ld, %ld\n",
        x1, x2, y2, y1, a, b, b1, dx, dy, err);
    do {
        gfx_draw_vline(lcd, x1, y2, x1, y1); /* III. to II. Quadrant */
        gfx_draw_vline(lcd, x2, y2, x2, y1); /* IV. to I. Quadrant */
        e2 = 2*err;
        if (e2 <= dy) { /* y step */
            y1++;
            y2--;
            dy += a;
            err += dy;
        }
        if (e2 >= dx || 2*err > dy) { /* x step */
            x1++;
            x2--;
            dx += b1;
            err += dx;
        }
    } while (x1 <= x2);

    while (y1-y2 < b) { /* too early stop of flat ellipses a=1 */
        gfx_draw_pixel(lcd, x1-1, y1); /* -> finish tip of ellipse */
        gfx_draw_pixel(lcd, x2+1, y1++);
        gfx_draw_pixel(lcd, x1-1, y2);
        gfx_draw_pixel(lcd, x2+1, y2--);
    }
}
