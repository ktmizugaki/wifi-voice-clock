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
#include <string.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include "misc.h"
#include "app_mode.h"

#include "menu/menu.h"

#define TAG "menu"

app_mode_t app_mode_menu(void)
{
    enum menu_result res = menu_main();
    return res == MENU_SHUTDOWN? APP_MODE_SUSPEND: APP_MODE_CLOCK;
}
