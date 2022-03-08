/* Copyright 2022 Kawashima Teruaki
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

#ifndef BATT_BMP_H
#define BATT_BMP_H

#include <gfx_bitmap.h>

#define BATT_BMP_SUBIMG 6
#define BATT_BMP_SUBWIDTH   (batt_bmp.header.width)
#define BATT_BMP_SUBHEIGHT  (batt_bmp.header.height/BATT_BMP_SUBIMG)
extern const gfx_bitmap_t batt_bmp;

#endif /* BATT_BMP_H */
