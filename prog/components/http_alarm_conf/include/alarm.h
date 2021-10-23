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

#ifdef CONFIG_NUM_ALARM
#define NUM_ALARM   CONFIG_NUM_ALARM
#else
#define NUM_ALARM   5
#endif

struct alarm {
    bool enabled;
    char name[11];
    uint8_t weeks;
    int seconds;
    int alarm_id;
};

extern esp_err_t alarm_init(void);
extern esp_err_t alarm_save(int index);
extern void alarm_set_num_alarm_sound(int num_alarm_sound);
extern int alarm_get_num_alarm_sound(void);

extern esp_err_t alarm_get_alarm(int index, const struct alarm **ppalarm);
extern esp_err_t alarm_get_alarms(const struct alarm **palarms, int *num_alarm);
extern esp_err_t alarm_set_alarm(int index, const struct alarm *alarm);

extern int64_t alarm_wakeup_us(struct tm *tm, suseconds_t *usec);
extern bool alarm_get_current_alarm(struct tm *tm, int range, const struct alarm **ppalarm);

#ifdef __cplusplus
}
#endif
