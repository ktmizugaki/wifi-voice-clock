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

#include <stdbool.h>
#include <esp_err.h>
#include <esp_log.h>

#include "app_display.h"
#include "power.h"
#include "app_mode.h"

#define TAG "suspend"

app_mode_t app_mode_suspend(void)
{
    ESP_LOGD(TAG, "handle_suspend");
    app_display_off();

    power_suspend();
}
