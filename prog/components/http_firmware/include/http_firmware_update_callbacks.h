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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * callbacks to interact with update process.
 */

/** @brief indicates firmware update result. */
enum firmware_update_result {
    FIRMWARE_UPDATE_OK = 0,
    FIRMWARE_UPDATE_E_FAIL,
    FIRMWARE_UPDATE_E_BUSY,
    FIRMWARE_UPDATE_E_LOWBATT,
};

/** @brief a set of callbackes to interact with firmware update process.
 * use @ref http_firmware_set_update_callbacks to set created callbacks.
 */
typedef struct {
    /** @brief called before starting firmware update.
     * this callback can be used to prevent firmware update depending on
     * application/system status.
     * @note this callback cannot be used to know start of firmware update.
     *   firmware update might not be started if other conditions does not met.
     * @return
     * - FIRMWARE_UPDATE_OK: allow start firmware update.
     * - FIRMWARE_UPDATE_E_BUSY: prevent update because system is busy.
     * - FIRMWARE_UPDATE_E_LOWBATT: prevent update because battery is low.
     * - FIRMWARE_UPDATE_E_FAIL: prevent update for other reasons.
     */
    enum firmware_update_result (*prestart)(void);
    /** @brief called when started firmware update.
     * use this callback to know start of firmware update.
     */
    void (*started)(void);
    /** @brief called when finished firmware update.
     * use this callback to know firmware update is finished.
     * application is expeceted to restart device.
     * @param result
     * - FIRMWARE_UPDATE_OK: successfuly updated.
     * - FIRMWARE_UPDATE_E_FAIL: failed to update.
     */
    void (*finished)(enum firmware_update_result result);
} firmware_update_callbacks_t;

/** @brief set a callbacks to interact with update process.
 * @note only last callbacks is used.
 * @note if no callbacks is set by this function,
 *   this component restarts device when finished firmware update.
 *   if a callbacks is set, this component does not restart device.
 */
extern void http_firmware_set_update_callbacks(const firmware_update_callbacks_t *callbacks);
#ifdef __cplusplus
}
#endif
