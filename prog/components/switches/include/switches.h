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

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

enum switch_flags {
    SWITCH_KEY1 = 1<<0,
    SWITCH_KEY2 = 1<<1,
    SWITCH_KEY3 = 1<<2,
    SWITCH_KEY4 = 1<<3,
    SWITCH_KEY5 = 1<<4,
    SWITCH_KEY6 = 1<<5,
    SWITCH_KEY7 = 1<<6,
    SWITCH_KEY8 = 1<<7,
    SWITCH_PRESS = 1<<13,
    SWITCH_RELEASE = 1<<14,
    SWITCH_HOLD = 1<<15,
};

#define SWITCH_MAX_KEY  12

typedef void (switches_callback_t)(enum switch_flags action, enum switch_flags prev_flags);

extern esp_err_t switches_init(switches_callback_t *callback, int pin_count, const uint8_t *pins);
extern void switches_deinit(void);
extern enum switch_flags switches_get_state(void);

#ifdef __cplusplus
}
#endif
