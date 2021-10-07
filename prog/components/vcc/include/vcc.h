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

/**
 * @file
 * measure battery voltage using ADC2.
 * @todo
 * TODO: measurement is done at after voltage regulator, so capped to 3.3V.
 *       study the way to measure battery voltage with minimum current leak.
 */

/** abstracted battery levels. */
typedef enum {
    VCC_LEVEL_UNKNOWN = -1, /**< failed to measure vcc. */
    VCC_LEVEL_OK = 0,       /**< battery is charged enough or charger is connected. */
    VCC_LEVEL_DECREASING,   /**< battery is slightly used. */
    VCC_LEVEL_DECREASING2,  /**< battery is more used. */
    VCC_LEVEL_WARNING,      /**< battery is low. */
    VCC_LEVEL_CRITICAL,     /**< battery is in danger voltage. */
} vcc_level_t;

/**
 * @brief initialize gpio and ADC2.
 * @return ESP_OK for success.
 */
extern esp_err_t vcc_init(void);
/**
 * @brief read vcc voltage using ADC2.
 * this function saves last read value(cache) for some periods and may return
 * that value. set nocache to true to get current voltage.
 *
 * since this function uses ADC2, this function fails if WiFi is in use.
 * @note voltage is measured at GPIO output, so capped to ~3300mV.
 * @param[out] vcc      pointer to int to store measured/cached voltage in mV.
 * @param[in]  nocache  set to true to get current vcc.
 * @return
 *   - ESP_OK for success.
 *   - ESP_ERR_TIMEOUT if ADC2 is locked by WiFi.
 *   - other value for other errors.
 */
extern esp_err_t vcc_read(int *vcc, bool nocache);
/**
 * @brief get last read vcc voltage.
 * @param[out] vcc      pointer to int to store last read voltage in mV.
 * @return
 *  - ESP_OK if voltage is ever read and stored to vcc.
 *  - ESP_FAIL if voltage was never read.
 *  - ESP_ERR_INVALID_ARG if vcc is NULL.
 */
extern esp_err_t vcc_get_last_value(int *vcc);
/**
 * @brief determin battery level from read vcc voltage.
 * set nocache to true to get current level. see @ref vcc_read for detail about cache.
 *
 * @param[in]  nocache  set to true to get current level.
 * @return one of @ref vcc_level_t.
 */
extern vcc_level_t vcc_get_level(bool nocache);

#if CONFIG_VCC_CHARGE_STAT_PIN >= 0
/** charging state. */
typedef enum {
    VCC_CHARG_DISCONNECTED, /**< charger or battery is disconnected. */
    VCC_CHARG_CHARGING,     /**< charging. */
    VCC_CHARG_COMPLETED,    /**< battery is fully charged. */
} vcc_charge_state_t;
/**
 * @brief get charging state.
 * @param[out] state        pointer to @ref vcc_charge_state_t to store charging state.
 * @return ESP_OK for success.
 */
extern esp_err_t vcc_get_charge_state(vcc_charge_state_t *state);
#endif

/**
 * @brief test function to expose ADC2 vref to GPIO 27.
 * equivalent to <code>adc2_vref_to_gpio(GPIO_NUM_27);</code>.
 *
 * use only this function in debug build.
 * @return ESP_OK for success.
 */
extern esp_err_t vcc_vref_enable(void);

#ifdef __cplusplus
}
#endif
