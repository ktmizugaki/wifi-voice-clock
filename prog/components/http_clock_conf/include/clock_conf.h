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
#include <stdint.h>
#include <time.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * handle clcok configuration.
 */

/** structure to hold clock conf. */
typedef struct {
    char TZ[8];         /**< timezone. */
    uint8_t sync_weeks; /**< bit flags of days of week to sync clock. */
    int32_t sync_time;  /**< time in day to sync clock, represented in seconds. */
} clock_conf_t;

/**
 * @brief initialize clock.
 * load clock conf from nvs, then set timezone. default values are used if failed to load.
 * @return ESP_OK.
 */
extern esp_err_t clock_conf_init(void);
/**
 * @brief get clock conf.
 * @param[out] conf pointer to conf copied to.
 */
extern void clock_conf_get(clock_conf_t *conf);
/**
 * @brief set and save clock conf.
 * @param[in] conf  pointer to conf.
 * @return
 *  - ESP_OK if successfully saved clock conf.
 *  - ESP_ERR_NVS_* for nvs error.
 */
extern esp_err_t clock_conf_set(const clock_conf_t *conf);

/**
 * @brief get micro seconds to next sync clock.
 * @param[in] tm        current time in tm.
 * @param[in] usec      deciaml part of current time in microsecond.
 * @return micro seconds to next sync time or
 *      INT64_MAX if it is not time to sync in next 24 hours.
 */
extern int64_t clock_conf_wakeup_us(struct tm *tm, suseconds_t *usec);
/**
 * @brief check if it is time to sync clock.
 * @param[in]  tm       current time in tm.
 * @param[in]  range    range of time to check.
 * @return true if it is time to play alarm, false otherwise.
 */
extern bool clock_conf_is_sync_time(struct tm *tm, int range);

#ifdef __cplusplus
}
#endif
