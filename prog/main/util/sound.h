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
#include <time.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

extern esp_err_t sound_ensure_init(void);
extern esp_err_t sound_play_repeat_notify(const char *name, int duration,
    void (*notify_end_func)(void));
extern esp_err_t sound_play_repeat(const char *name, int duration);
extern esp_err_t sound_play(const char *name);
extern bool sound_is_playing(void);
extern void sound_stop(void);

#ifdef __cplusplus
}
#endif
