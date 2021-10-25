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
#include <vcc.h>

#include "app_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_USE_SYSLOG
extern void misc_ensure_init_udplog(void);
extern void misc_udplog_vcc(void);
#endif

extern void misc_ensure_vcc_level(vcc_level_t min_level, bool is_interactive);

extern bool misc_process_time_task(void);
extern void misc_handle_event(const app_event_t *event);

#ifdef __cplusplus
}
#endif
