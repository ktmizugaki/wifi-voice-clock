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

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * handle tactile switches.
 * this notifies changes of switches with features:
 * * debounce.
 * * detect hold.
 *
 * @note this module assumes gpio pins are pulled down externally and
 *       the other end of switches are connected to VCC.
 */

/** ORed flags indicating switch state and action. */
enum switch_flags {
    SWITCH_KEY1 = 1<<0,     /**< state indicates key 1 is on. */
    SWITCH_KEY2 = 1<<1,     /**< state indicates key 2 is on. */
    SWITCH_KEY3 = 1<<2,     /**< state indicates key 3 is on. */
    SWITCH_KEY4 = 1<<3,     /**< state indicates key 4 is on. */
    SWITCH_KEY5 = 1<<4,     /**< state indicates key 5 is on. */
    SWITCH_KEY6 = 1<<5,     /**< state indicates key 6 is on. */
    SWITCH_KEY7 = 1<<6,     /**< state indicates key 7 is on. */
    SWITCH_KEY8 = 1<<7,     /**< state indicates key 8 is on. */
    SWITCH_PRESS = 1<<13,   /**< action sent when key is pressed down. */
    SWITCH_RELEASE = 1<<14, /**< action sent when key is released. */
    SWITCH_HOLD = 1<<15,    /**< state indicates keys are being hold. also sent as action when held. */
};

/** @brief maximum number of keys. */
#define SWITCH_MAX_KEY  12

/**
 * @brief type of callback function to notify key state change.
 * @param[in] action        combination of keys and with one of actions: SWITCH_PRESS, SWITCH_RELEASE, SWITCH_HOLD
 * @param[in] prev_flags    combination of switch flags before this action occurs.
 */
typedef void (switches_callback_t)(enum switch_flags action, enum switch_flags prev_flags);

/**
 * @brief initialize switches with pins.
 * initialize gpios from pins and start task to notify switch state changes.
 * pins are array of gpio numbers corresponds to KEY flags.
 * pins[0] is SWITCH_KEY1, pins[1] is SWITCH_KEY2, and so on.
 * @param[in] callback      callback function that receives switch state changes.
 * @param[in] pin_count     length of pins parameter.
 * @param[in] pins          gpio numbers of pins to use.
 * @return ESP_OK for success.
 */
extern esp_err_t switches_init(switches_callback_t *callback, int pin_count, const uint8_t *pins);
/**
 * @brief deinitialize switches.
 * stop notifying switch state changes.
 */
extern void switches_deinit(void);
/**
 * @brief return current combination of pressed keys and held status.
 * @return current combination of pressed keys and held status.
 */
extern enum switch_flags switches_get_state(void);

#ifdef __cplusplus
}
#endif
