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

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

enum app_sync_state {
    APP_SYNC_WIFI,
    APP_SYNC_SNTP,
    APP_SYNC_FAIL,
    APP_SYNC_SUCCESS,
};

extern esp_err_t app_clock_init(void);
extern esp_err_t app_clock_start_sync(void);
extern void app_clock_stop_sync(void);
extern bool app_clock_is_done(void);

#ifdef __cplusplus
}
#endif
