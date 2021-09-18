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

#ifdef __cplusplus
extern "C" {
#endif

enum app_action {
    APP_ACTION_NONE = 0,
    APP_ACTION_LEFT = 0x0002,
    APP_ACTION_RIGHT = 0x0001,
    APP_ACTION_MIDDLE = 0x0004,
    APP_ACTION_FLAG_PRESS = 0x2000,
    APP_ACTION_FLAG_RELEASE = 0x4000,
    APP_ACTION_FLAG_LONG = 0x8000,
};

extern void app_switches_enable_wake(void);
extern int app_switches_check_wake(void);
extern esp_err_t app_switches_init(void);
extern void app_switches_wait_up(void);
extern enum app_action app_switches_get_state(void);

#ifdef __cplusplus
}
#endif
