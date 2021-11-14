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
#include <sys/time.h>
#include <time.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * handle alarm configuration.
 */

/** number of alarms. */
#ifdef CONFIG_NUM_ALARM
#define NUM_ALARM   CONFIG_NUM_ALARM
#else
#define NUM_ALARM   5
#endif

/** alarm setting. */
struct alarm {
    bool enabled;   /**< flag to enable/disable alarm. */
    char name[11];  /**< name of alarm */
    uint8_t weeks;  /**< bit flags of days of week to play alarm. */
    int seconds;    /**< time in day to play alarm, represented in seconds. */
    int alarm_id;   /**< id of alarm sound */
};

/** load alarms from nvs. */
extern esp_err_t alarm_init(void);
/** set number of alarm sounds. */
extern void alarm_set_num_alarm_sound(int num_alarm_sound);
/** get number of alarm sounds. */
extern int alarm_get_num_alarm_sound(void);

/**
 * @brief get alarm at index.
 * @param[in]  index    index of alarm.
 * @param[out] ppalarm  pointer to set pointer to alarm.
 * @return
 *   - ESP_OK for success, ppalarm is set to pointer to alarm.
 *   - ESP_ERR_INVALID_ARG if index is out of range or ppalarm is NULL.
 */
extern esp_err_t alarm_get_alarm(int index, const struct alarm **ppalarm);
/**
 * @brief get alarm at index.
 * @param[out] palarms      pointer to array of alarms.
 * @param[out] num_alarm    number of alarms.
 * @return ESP_OK for success.
 */
extern esp_err_t alarm_get_alarms(const struct alarm **palarms, int *num_alarm);
/**
 * @brief set and save alarm at index.
 * @param[in] index     index of alarm.
 * @param[in] alarm     pointer to set pointer to alarm.
 * @return
 *  - ESP_OK for successfully saved alarm.
 *  - ESP_ERR_INVALID_ARG if index is out of range or alarm is NULL.
 *  - ESP_ERR_NVS_* for nvs error.
 */
extern esp_err_t alarm_set_alarm(int index, const struct alarm *alarm);

/**
 * @brief get micro seconds to next alarm time.
 * @param[in] tm        current time in tm.
 * @param[in] usec      deciaml part of current time in microsecond.
 * @return micro seconds to next alarm time or
 *      INT64_MAX if alarm is not scheduled in next 24 hours.
 */
extern int64_t alarm_wakeup_us(struct tm *tm, suseconds_t *usec);
/**
 * @brief check if it is time to play alarm.
 * @param[in]  tm       current time in tm.
 * @param[in]  range    range of time to check.
 * @param[out] ppalarm  set to current alarm when return value is true.
 * @return true if it is time to play alarm, false otherwise.
 */
extern bool alarm_get_current_alarm(struct tm *tm, int range, const struct alarm **ppalarm);

#ifdef __cplusplus
}
#endif
