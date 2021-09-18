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

typedef enum {
    APP_EVENT_ACTION,
    APP_EVENT_CLOCK,
    APP_EVENT_SYNC,
} app_event_id_t;

typedef struct {
    app_event_id_t id;
    int arg0;
    int arg1;
} app_event_t;

extern esp_err_t app_event_init(void);
extern void app_event_send(app_event_id_t event_id);
extern void app_event_send_arg(app_event_id_t event_id, int arg);
extern void app_event_send_args(app_event_id_t event_id, int arg0, int arg1);
extern bool app_event_get(app_event_t *ev);
extern bool app_event_get_to(app_event_t *ev, int timeout_ms);

#ifdef __cplusplus
}
#endif
