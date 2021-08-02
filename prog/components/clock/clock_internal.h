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

#include <esp_event.h>

#include "clock.h"
#include "clock_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void clock_sync_sntp_process(void);
extern void clock_event_post(clock_event_t event, void *data, size_t data_size);

#ifdef __cplusplus
}
#endif
