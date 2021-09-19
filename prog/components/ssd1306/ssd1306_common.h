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

#define SSD1306_X2COL(x)    (x)
#define SSD1306_Y2ROW(y)    ((y)/8)
#define SSD1306_XY2POS(device, x, y)    (SSD1306_Y2ROW(y)+SSD1306_X2COL(x)*SSD1306_ROWS(device->height))
#define SSD1306_XY2BIT(device, x, y)    (((uint8_t)1)<<((y)&7))

#define SSD1306_COLUMNS(width)  (width)
#define SSD1306_ROWS(height)    (((height)+7)/8)
#define SSD1306_BUFSIZE(width, height)  (SSD1306_ROWS(height)*SSD1306_COLUMNS(width))

/* 1. Fundamental Command Table */
/** Set Contrast Control
 * Double byte command to select 1 out of 256 contrast steps. Contrast
 * increases as the value increases. (RESET = 7Fh ) */
#define SSD1306_CMD_SETCONTRAST(val) 0x81, (val)
/** Entire Display ON
 * val=false: Resume to RAM content display (RESET).
 * Output follows RAM content
 * val=true: Entire display ON.
 * Output ignores RAM content */
#define SSD1306_CMD_ENTIREDISPLAYON(val) (0xA4|((val)?1:0))
/** Set Normal/Inverse Display
 * val=false: Normal display (RESET)
 * 0 in RAM: OFF in display panel
 * 1 in RAM: ON in display panel
 * val=true: Inverse display
 * 0 in RAM: ON in display panel
 * 1 in RAM: OFF in display panel */
#define SSD1306_CMD_SETINVERTDISPLAY(val) (0xA6|((val)?1:0))
/** Set Display ON/OFF
 * val=false: 0b:Display OFF (sleep mode) (RESET)
 * val=true:Display ON in normal mode */
#define SSD1306_CMD_SETDISPLAYON(val) (0xAE|((val)?1:0))

/* 2. Scrolling Command Table */
/** Continuous Horizontal Scroll Setup
 * toleft=false: Right Horizontal Scroll
 * toleft=true: Left Horizontal Scroll
 * (Horizontal scroll by 1 column)
 * ps: Define start page address
 * 000b – PAGE0 011b – PAGE3 110b – PAGE6
 * 001b – PAGE1 100b – PAGE4 111b – PAGE7
 * 010b – PAGE2 101b – PAGE5
 * pe: Define end page address
 * 000b – PAGE0 011b – PAGE3 110b – PAGE6
 * 001b – PAGE1 100b – PAGE4 111b – PAGE7
 * 010b – PAGE2 101b – PAGE5
 * The value of pe must be larger or equal to ps
 * int: Set time interval between each scroll step in terms of frame frequency
 * 000b – 5 frames 100b – 3 frames
 * 001b – 64 frames 101b – 4 frames
 * 010b – 128 frames 110b – 25 frame
 * 011b – 256 frames 111b – 2 frame */
#define SSD1306_CMD_HOZSCROLLL(toleft, ps, pe, int) 0x26|(toleft?1:0), 0x00, (ps)&7, (int)&7, (pe)&7, 0x00, 0xff
#define SSD1306_CMD_RIGHTHOZSCROLLL(ps, pe, int) SSD1306_CMD_HOZSCROLLL(0, (ps), (pe), (int))
#define SSD1306_CMD_LEFTHOZSCROLLL(ps, pe, int) SSD1306_CMD_HOZSCROLLL(1, (ps), (pe), (int))
/** Continuous Vertical and Horizontal Scroll Setup
 * toleft=false: Right Horizontal Scroll
 * toleft=true: Left Horizontal Scroll
 * (Horizontal scroll by 1 column)
 * ps: Define start page address
 * 000b – PAGE0 011b – PAGE3 110b – PAGE6
 * 001b – PAGE1 100b – PAGE4 111b – PAGE7
 * 010b – PAGE2 101b – PAGE5
 * pe: Define end page address
 * 000b – PAGE0 011b – PAGE3 110b – PAGE6
 * 001b – PAGE1 100b – PAGE4 111b – PAGE7
 * 010b – PAGE2 101b – PAGE5
 * The value of pe must be larger or equal to ps
 * int: Set time interval between each scroll step in terms of frame frequency
 * 000b – 5 frames 100b – 3 frames
 * 001b – 64 frames 101b – 4 frames
 * 010b – 128 frames 110b – 25 frame
 * 011b – 256 frames 111b – 2 frame
 * os: Vertical scrolling offset
 * e.g. os= 01h refer to offset =1 row
 * os =3Fh refer to offset =63 rows
 * Note
 * (1) No continuous vertical scrolling is available. */
#define SSD1306_CMD_VERTHOZSCROLLL(toleft, ps, pe, int, os) 0x28|(toleft?2:1), 0x00, (ps)&7, (int)&7, (pe)&7, (os)&0x3f
#define SSD1306_CMD_VERTRIGHTHOZSCROLLL(ps, pe, int) SSD1306_CMD_VERTHOZSCROLLL(0, (ps), (pe), (int), (os))
#define SSD1306_CMD_VERTLEFTHOZSCROLLL(ps, pe, int) SSD1306_CMD_VERTHOZSCROLLL(1, (ps), (pe), (int), (os))
/** Deactivate scroll
 * Stop scrolling that is configured by command
 * 26h/27h/29h/2Ah.
 * Note
 * (1) After sending 2Eh command to deactivate the scrolling
 * action, the ram data needs to be rewritten. */
#define SSD1306_CMD_DEACTIVATESCROLL 0x2E
/** Activate scroll
 * Start scrolling that is configured by the scrolling setup
 * commands :26h/27h/29h/2Ah with the following valid
 * sequences:
 * Valid command sequence 1: 26h ;2Fh.
 * Valid command sequence 2: 27h ;2Fh.
 * Valid command sequence 3: 29h ;2Fh.
 * Valid command sequence 4: 2Ah ;2Fh.
 * For example, if “26h; 2Ah; 2Fh.” commands are
 * issued, the setting in the last scrolling setup command,
 * i.e. 2Ah in this case, will be executed. In other words,
 * setting in the last scrolling setup command overwrites
 * the setting in the previous scrolling setup commands. */
#define SSD1306_CMD_ACTIVATESCROLL 0x2F
/** Set Vertical Scroll Area
 * tf: Set No. of rows in top fixed area. The No. of rows in top fixed area
 * is referenced to the top of the GDDRAM (i.e. row 0). [RESET = 0]
 * sa: Set No. of rows in scroll area. This is the number of rows to be used
 * for vertical scrolling. The scroll area starts in the first row below the
 * top fixed area. [RESET = 64]
 * Note
 * (1) tf+sa <= MUX ratio
 * (2) sa <= MUX ratio
 * (3a) Vertical scrolling offset (os in 29h/2Ah) < sa
 * (3b) Set Display Start Line (sl of 40h~7Fh) < sa
 * (4) The last row of the scroll area shifts to the first row of the scroll
 * area.
 * (5) For 64d MUX display
 * tf = 0, sa=64 : whole area scrolls
 * tf = 0, sa < 64 : top area scrolls
 * tf + sa < 64 : central area scrolls
 * tf + sa = 64 : bottom area scrolls */
#define SSD1306_CMD_SETVERTSCROLLAREA(tf, sa) 0xA3, (tf)&0x3f, (sa)&0x7f

/* 3. Addressing Setting Command Table */
/** Set Lower Column Start Address for Page Addressing Mode
 * Set the lower nibble of the column start address register for Page
 * Addressing Mode using val as data bits. The initial display line register
 * is reset to 0000b after RESET.
 * Note
 * (1) This command is only for page addressing mode */
#define SSD1306_CMD_SETLCSAFORPAM(val) ((val)&0x0f)
/** Set Higher Column Start Address for Page Addressing Mode
 * Set the higher nibble of the column start address register for Page
 * Addressing Mode using val as data bits. The initial display line register
 * is reset to 0000b after RESET.
 * Note
 * (1) This command is only for page addressing mode */
#define SSD1306_CMD_SETHCSAFORPAM(val) (0x10|((val)&0x0f))
/** Set Memory Addressing Mode
 * val = 00b, Horizontal Addressing Mode
 * val = 01b, Vertical Addressing Mode
 * val = 10b, Page Addressing Mode (RESET)
 * val = 11b, Invalid */
#define SSD1306_CMD_SETMAM(val) 0x20, (val)&3
/** Set Column Address
 * Setup column start and end address
 * cs: Column start address, range : 0-127d, (RESET=0d)
 * ce: Column end address, range : 0-127d, (RESET =127d)
 * Note
 * (1) This command is only for horizontal or vertical addressing mode. */
#define SSD1306_CMD_SETCOLADDR(cs, ce) 0x21, (cs)&0x7f, (ce)&0x7f
/** Set Page Address Setup page start and end address
 * ps : Page start Address, range : 0-7d, (RESET = 0d)
 * pe : Page end Address, range : 0-7d, (RESET = 7d)
 * Note
 * (1) This command is only for horizontal or vertical addressing mode. */
#define SSD1306_CMD_SETPAGEADDR(ps, pe) 0x22, (ps)&0x7, (pe)&0x7
/** Set Page Start Address for Page Addressing Mode
 * Set GDDRAM Page Start Address (PAGE0~PAGE7) for Page Addressing Mode
 * using val.
 * Note
 * (1) This command is only for page addressing mode */
#define SSD1306_CMD_SETPAGESTARTADDR(val) (0xB0|((val)&7))

/* 4. Hardware Configuration (Panel resolution & layout related) Command Table */
/** Set Display Start Line
 * Set display RAM display start line register from 0-63 using sl.
 * Display start line register is reset to 000000b during RESET. */
#define SSD1306_CMD_SETDISPLAYSTARTLINE(sl) (0x40|(sl&0x3f))
/** Set Segment Re-map
 * val=false: column address 0 is mapped to SEG0 (RESET)
 * val=true: column address 127 is mapped to SEG0
 */
#define SSD1306_CMD_SETSEGMENTREMAP(val) (0xA0|(val?1:0))
/** Set Multiplex Ratio
 * Set MUX ratio to N+1 MUX
 * N : from 16MUX to 64MUX, RESET=111111b (i.e. 63d, 64MUX)
 * N from 0 to 14 are invalid entry. */
#define SSD1306_CMD_SETMUXRATIO(N) 0xA8, N&0x3f
/** Set COM Output Scan Direction
 * val=false: normal mode (RESET) Scan from COM0 to COM[N –1]
 * val=true: remapped mode. Scan from COM[N-1] to COM0
 * Where N is the Multiplex ratio. */
#define SSD1306_CMD_SETCOMOUTSCANDIR(val) (0xC0|(val?8:0))
/** Set Display Offset
 * Set vertical shift by COM from 0d~63d
 * The value is reset to 00h after RESET. */
#define SSD1306_CMD_SETDISPLAYOFFSET(val) 0xD3, val&0x3f
/** Set COM Pins Hardware Configuration
 * val1=false, Sequential COM pin configuration
 * val1=true(RESET), Alternative COM pin configuration
 * val2=false(RESET), Disable COM Left/Right remap
 * val2=true, Enable COM Left/Right remap */
#define SSD1306_CMD_SETCOMPINSHWCONF(val1, val2) 0xDA, 0x02|(val1?1<<4:0)|(val2?1<<5:0)

/* 5. Timing & Driving Scheme Setting Command Table */
/** Set Display Clock Divide Ratio/Oscillator Frequency
 * div : Define the divide ratio (D) of the display clocks (DCLK):
 * Divide ratio= div + 1, RESET is 0000b (divide ratio = 1)
 * freq: Set the Oscillator Frequency, F{OSC}. Oscillator Frequency increases
 * with the value of div and vice versa. RESET is 1000b
 * Range:0000b~1111b
 * Frequency increases as setting value increases. */
#define SSD1306_CMD_SETCLOCKDIVOSCFREQ(div, freq) 0xD5,(div&0xf)|((freq&0xf)<<4)
/** Set Pre-charge Period
 * phase1: Phase 1 period of up to 15 DCLK
 *   clocks 0 is invalid entry
 *   (RESET=2h)
 * phase2: Phase 2 period of up to 15 DCLK
 *   clocks 0 is invalid entry
 *   (RESET=2h) */
#define SSD1306_CMD_SETPRECHAEGEPERIOD(phase1, phase2) 0xD9,(phase1&0xf)|((phase2&0xf)<<4)
/** Set V{COMH} Deselect Level
 * lvl=000b: ~ 0.65 x V{CC}
 * lvl=010b: ~ 0.77 x V{CC} (RESET)
 * lvl=011b: ~ 0.83 x V{CC} */
#define SSD1306_CMD_SETVCOMHDESELECTLEVEL(lvl) 0xDB,(lvl&0x7)<<4
/** NOP
 * Command for no operation */
#define SSD1306_CMD_NOP 0xE3

/* ??? */
#define SSD1306_CMD_SETCHARGEPUMP(val) 0x8D,0x10|(val?4:0)
