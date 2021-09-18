/* https://github.com/ArminJo/STMF3-Discovery-Demos/blob/master/lib/graphics/src/thickLine.cpp */
/*
 * thickLine.cpp
 * Draw a solid line with thickness using a modified Bresenhams algorithm.
 *
 * @date 25.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmail.com
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include <stdint.h>

#include "lcd.h"
#include "gfx_primitive.h"
#include "trace.h"

/*
 * Overlap means drawing additional pixel when changing minor direction
 * Needed for drawThickLine, otherwise some pixels will be missing in the thick line
 */

#define LINE_OVERLAP_NONE 0 	// No line overlap, like in standard Bresenham
#define LINE_OVERLAP_MAJOR 0x01 // Overlap - first go major then minor direction. Pixel is drawn as extension after actual line
#define LINE_OVERLAP_MINOR 0x02 // Overlap - first go minor then major direction. Pixel is drawn as extension before next line
#define LINE_OVERLAP_BOTH 0x03  // Overlap - both

/**
 * Modified Bresenham draw(line) with optional overlap. Required for drawThickLine().
 * Overlap draws additional pixel when changing minor direction. For standard bresenham overlap, choose LINE_OVERLAP_NONE (0).
 *
 *  Sample line:
 *
 *    00+
 *     -0000+
 *         -0000+
 *             -00
 *
 *  0 pixels are drawn for normal line without any overlap
 *  + pixels are drawn if LINE_OVERLAP_MAJOR
 *  - pixels are drawn if LINE_OVERLAP_MINOR
 */
static void drawLineOverlap(abstract_lcd_t *lcd, int aXStart, int aYStart, int aXEnd, int aYEnd, int aOverlap)
{
    int16_t tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    if ((aXStart == aXEnd) || (aYStart == aYEnd)) {
        //horizontal or vertical line -> fillRect() is faster than drawLine()
        gfx_fill_rect(lcd, aXStart, aYStart, aXEnd, aYEnd);
    } else {
        //calculate direction
        tDeltaX = aXEnd - aXStart;
        tDeltaY = aYEnd - aYStart;
        if (tDeltaX < 0) {
            tDeltaX = -tDeltaX;
            tStepX = -1;
        } else {
            tStepX = +1;
        }
        if (tDeltaY < 0) {
            tDeltaY = -tDeltaY;
            tStepY = -1;
        } else {
            tStepY = +1;
        }
        tDeltaXTimes2 = tDeltaX << 1;
        tDeltaYTimes2 = tDeltaY << 1;
        //draw start pixel
        gfx_draw_pixel(lcd, aXStart, aYStart);
        if (tDeltaX > tDeltaY) {
            // start value represents a half step in Y direction
            tError = tDeltaYTimes2 - tDeltaX;
            while (aXStart != aXEnd) {
                // step in main direction
                aXStart += tStepX;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // draw pixel in main direction before changing
                        gfx_draw_pixel(lcd, aXStart, aYStart);
                    }
                    // change Y
                    aYStart += tStepY;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // draw pixel in minor direction before changing
                        gfx_draw_pixel(lcd, aXStart - tStepX, aYStart);
                    }
                    tError -= tDeltaXTimes2;
                }
                tError += tDeltaYTimes2;
                gfx_draw_pixel(lcd, aXStart, aYStart);
            }
        } else {
            tError = tDeltaXTimes2 - tDeltaY;
            while (aYStart != aYEnd) {
                aYStart += tStepY;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // draw pixel in main direction before changing
                        gfx_draw_pixel(lcd, aXStart, aYStart);
                    }
                    aXStart += tStepX;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // draw pixel in minor direction before changing
                        gfx_draw_pixel(lcd, aXStart, aYStart - tStepY);
                    }
                    tError -= tDeltaYTimes2;
                }
                tError += tDeltaXTimes2;
                gfx_draw_pixel(lcd, aXStart, aYStart);
            }
        }
    }
}

/**
 * Bresenham with thickness
 * No pixel missed and every pixel only drawn once!
 * The code is bigger and more complicated than drawThickLineSimple() but it tends to be faster, since drawing a pixel is often a slow operation.
 * aThicknessMode can be one of LINE_THICKNESS_MIDDLE, LINE_THICKNESS_DRAW_CLOCKWISE, LINE_THICKNESS_DRAW_COUNTERCLOCKWISE
 */
static void drawThickLine(abstract_lcd_t *lcd, int aXStart, int aYStart, int aXEnd, int aYEnd, int aThickness)
{
    int16_t i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    if (aThickness <= 1) {
        drawLineOverlap(lcd, aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE);
        return;
    }

    /**
     * For coordinate system with 0.0 top left
     * Swap X and Y delta and calculate clockwise (new delta X inverted)
     * or counterclockwise (new delta Y inverted) rectangular direction.
     * The right rectangular direction for LINE_OVERLAP_MAJOR toggles with each octant
     */
    tDeltaY = aXEnd - aXStart;
    tDeltaX = aYEnd - aYStart;
    // mirror 4 quadrants to one and adjust deltas and stepping direction
    int tSwap = 1; // count effective mirroring
    if (tDeltaX < 0) {
        tDeltaX = -tDeltaX;
        tStepX = -1;
        tSwap = !tSwap;
    } else {
        tStepX = +1;
    }
    if (tDeltaY < 0) {
        tDeltaY = -tDeltaY;
        tStepY = -1;
        tSwap = !tSwap;
    } else {
        tStepY = +1;
    }
    tDeltaXTimes2 = tDeltaX << 1;
    tDeltaYTimes2 = tDeltaY << 1;
    int tOverlap;
    // adjust for right direction of thickness from line origin
    int tDrawStartAdjustCount = aThickness / 2;

    // which octant are we now
    if (tDeltaX >= tDeltaY) {
        if (tSwap) {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        } else {
            tStepX = -tStepX;
        }
        /*
         * Vector for draw direction of start of lines is rectangular and counterclockwise to main line direction
         * Therefore no pixel will be missed if LINE_OVERLAP_MAJOR is used on change in minor rectangular direction
         */
        // adjust draw start point
        tError = tDeltaYTimes2 - tDeltaX;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            // change X (main direction here)
            aXStart -= tStepX;
            aXEnd -= tStepX;
            if (tError >= 0) {
                // change Y
                aYStart -= tStepY;
                aYEnd -= tStepY;
                tError -= tDeltaXTimes2;
            }
            tError += tDeltaYTimes2;
        }
        //draw start line
        drawLineOverlap(lcd, aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE);
        // draw aThickness number of lines
        tError = tDeltaYTimes2 - tDeltaX;
        for (i = aThickness; i > 1; i--) {
            // change X (main direction here)
            aXStart += tStepX;
            aXEnd += tStepX;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                // change Y
                aYStart += tStepY;
                aYEnd += tStepY;
                tError -= tDeltaXTimes2;
                /*
                 * Change minor direction reverse to line (main) direction
                 * because of choosing the right (counter)clockwise draw vector
                 * Use LINE_OVERLAP_MAJOR to fill all pixel
                 *
                 * EXAMPLE:
                 * 1,2 = Pixel of first 2 lines
                 * 3 = Pixel of third line in normal line mode
                 * - = Pixel which will additionally be drawn in LINE_OVERLAP_MAJOR mode
                 *           33
                 *       3333-22
                 *   3333-222211
                 * 33-22221111
                 *  221111                     /\
				 *  11                          Main direction of start of lines draw vector
                 *  -> Line main direction
                 *  <- Minor direction of counterclockwise of start of lines draw vector
                 */
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaYTimes2;
            drawLineOverlap(lcd, aXStart, aYStart, aXEnd, aYEnd, tOverlap);
        }
    } else {
        // the other octant
        if (tSwap) {
            tStepX = -tStepX;
        } else {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        }
        // adjust draw start point
        tError = tDeltaXTimes2 - tDeltaY;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            aYStart -= tStepY;
            aYEnd -= tStepY;
            if (tError >= 0) {
                aXStart -= tStepX;
                aXEnd -= tStepX;
                tError -= tDeltaYTimes2;
            }
            tError += tDeltaXTimes2;
        }
        //draw start line
        drawLineOverlap(lcd, aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE);
        // draw aThickness number of lines
        tError = tDeltaXTimes2 - tDeltaY;
        for (i = aThickness; i > 1; i--) {
            aYStart += tStepY;
            aYEnd += tStepY;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                aXStart += tStepX;
                aXEnd += tStepX;
                tError -= tDeltaYTimes2;
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaXTimes2;
            drawLineOverlap(lcd, aXStart, aYStart, aXEnd, aYEnd, tOverlap);
        }
    }
}

void gfx_draw_thick_line(abstract_lcd_t *lcd,
    int x1, int y1, int x2, int y2, int thickness)
{
    if (thickness <= 0) {
        TRACE("thick line: invalid thickness: %d\n", thickness);
        return;
    }
    if (thickness == 1) {
        TRACE("thick line: draw line\n");
        gfx_draw_line(lcd, x1, y1, x2, y2);
        return;
    }
    if (x1 == x2 && y1 == y2) {
        TRACE("thick line: draw line\n");
        gfx_draw_line(lcd, x1-thickness/2, y1, x2+thickness/2, y2);
        return;
    }
    if (x1 == x2) {
        if (y1 < y2) {
            x2 += thickness/2;
            x1 = x2 - thickness + 1;
        } else {
            x1 -= thickness/2;
            x2 = x1 + thickness - 1;
        }
        TRACE("thick line: fill rect: %d, %d, %d, %d\n", x1, y1, x2, y2);
        gfx_fill_rect(lcd, x1, y1, x2, y2);
        return;
    }
    if (y1 == y2) {
        if (x1 < x2) {
            y1 -= thickness/2;
            y2 = y1 + thickness - 1;
        } else {
            y2 += thickness/2;
            y1 = y2 - thickness + 1;
        }
        TRACE("thick line: fill rect: %d, %d, %d, %d\n", x1, y1, x2, y2);
        gfx_fill_rect(lcd, x1, y1, x2, y2);
        return;
    }
    drawThickLine(lcd, x1, y1, x2, y2, thickness);
}
