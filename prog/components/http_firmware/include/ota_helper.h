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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * small helper of app_update.
 */

/** @brief print OTA state of current firmware to stdout. */
extern void ota_print_partition_info(void);
/**
 * @brief check if it is needed to do self test and mark valid.
 * @return true if partition state is NEW or UNDEFINED.
 */
extern bool ota_need_self_test(void);
/**
 * @brief check if firmware can be rolled back.
 * @return true if there is another bootable firmware.
 * @note this function is same as esp_ota_check_rollback_is_possible.
 */
extern bool ota_can_rollback(void);
/** @brief mark current firmware as valid. */
extern void ota_mark_valid(void);
/** @brief rollback to previous firmware by marking current firmware as invalid and restart. */
extern void ota_rollback(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
