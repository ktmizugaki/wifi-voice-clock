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
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <nvs.h>
#include <esp_log.h>

#include "alarm.h"

#define TAG "alarm"
#define NVSKEY "alarm"

static bool s_loaded_alarms = false;
static int s_num_alarm_sound = 1;
static struct alarm s_alarms[NUM_ALARM];

struct alarm_packed {
    char name[11];
    uint8_t weeks;
    uint8_t alarm_id;
    uint8_t reserved;
    int seconds;
};

static void unpack_alarm(const struct alarm_packed *ppacked, struct alarm *palarm)
{
    palarm->enabled = ppacked->seconds>=0;
    strcpy(palarm->name, ppacked->name);
    palarm->weeks = ppacked->weeks;
    palarm->seconds = ppacked->seconds>=0?ppacked->seconds:~ppacked->seconds;
    palarm->alarm_id = ppacked->alarm_id;
}

static void pack_alarm(const struct alarm *palarm, struct alarm_packed *ppacked)
{
    strcpy(ppacked->name, palarm->name);
    ppacked->weeks = palarm->weeks;
    ppacked->alarm_id = palarm->alarm_id;
    ppacked->seconds = palarm->enabled?palarm->seconds:~palarm->seconds;
}

static esp_err_t alarm_load_alarm(int index, struct alarm *palarm, nvs_handle_t *pnvsh)
{
    esp_err_t err;
    nvs_handle_t nvsh;
    char key[4];
    struct alarm_packed packed;
    size_t length;
    if (pnvsh == NULL) {
        err = nvs_open(NVSKEY, NVS_READONLY, &nvsh);
        if (err != ESP_OK) {
            return err;
        }
    } else {
        nvsh = *pnvsh;
    }
    snprintf(key, sizeof(key), "%d", index);
    length = sizeof(struct alarm_packed);
    err = nvs_get_blob(nvsh, key, &packed, &length);
    if (length != sizeof(struct alarm_packed)) {
        err = ESP_ERR_NVS_NOT_FOUND;
    }
    if (err == ESP_OK) {
        unpack_alarm(&packed, palarm);
    }
    if (pnvsh == NULL) {
        nvs_close(nvsh);
    }
    return err;
}

static esp_err_t alarm_save_alarm(int index, const struct alarm *palarm, nvs_handle_t *pnvsh)
{
    esp_err_t err;
    nvs_handle_t nvsh;
    char key[4];
    struct alarm_packed packed;
    if (pnvsh == NULL) {
        err = nvs_open(NVSKEY, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) {
            return err;
        }
    } else {
        nvsh = *pnvsh;
    }
    snprintf(key, sizeof(key), "%d", index);
    pack_alarm(palarm, &packed);
    err = nvs_set_blob(nvsh, key, &packed, sizeof(struct alarm_packed));
    if (pnvsh == NULL) {
        if (err == ESP_OK) {
            err = nvs_commit(nvsh);
        }
        nvs_close(nvsh);
    }
    return err;
}

static esp_err_t alarm_load_all(void)
{
    bool storage = true;
    int i;
    nvs_handle nvsh;
    esp_err_t err;

    err = nvs_open(NVSKEY, NVS_READONLY, &nvsh);
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "nvs open failed");
        }
        storage = false;
    }
    for (i = 0; i < NUM_ALARM; i++) {
        struct alarm *palarm = &s_alarms[i];
        if (storage) {
            err = alarm_load_alarm(i, palarm, &nvsh);
        } else {
            err = ESP_ERR_NVS_NOT_FOUND;
        }
        if (err != ESP_OK) {
            palarm->enabled = false;
            memset(palarm->name, ' ', sizeof(palarm->name)-1);
            palarm->name[sizeof(palarm->name)-1] = 0;
            palarm->weeks = 0x7f;
            palarm->seconds = 0;
            palarm->alarm_id = 0;
        }
    }
    if (storage) {
        nvs_close(nvsh);
    }

    return ESP_OK;
}

static esp_err_t alarm_save_all(void)
{
    int i;
    nvs_handle nvsh;
    esp_err_t err;

    err = nvs_open(NVSKEY, NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }
    for (i = 0; i < NUM_ALARM; i++) {
        struct alarm *palarm = &s_alarms[i];
        err = alarm_save_alarm(i, palarm, &nvsh);
        if (err != ESP_OK) {
            break;
        }
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvsh);
    }
    nvs_close(nvsh);

    return err;
}

esp_err_t alarm_init(void)
{
    esp_err_t err;

    if (s_loaded_alarms) {
        return ESP_OK;
    }

    err = alarm_load_all();
    if (err == ESP_OK) {
        s_loaded_alarms = true;
    }

    return err;
}

esp_err_t alarm_save(int index)
{
    if (index < -1) {
        return alarm_save_all();
    } else {
        if (index < 0 || index >= NUM_ALARM) {
            return ESP_ERR_INVALID_ARG;
        }
        return alarm_save_alarm(index, &s_alarms[index], NULL);
    }
}

void alarm_set_num_alarm_sound(int num_alarm_sound)
{
    if (num_alarm_sound > 0) {
       s_num_alarm_sound = num_alarm_sound;
    }
}

int alarm_get_num_alarm_sound(void)
{
    return s_num_alarm_sound;
}

esp_err_t alarm_get_alarm(int index, const struct alarm **ppalarm)
{
    if (index < 0 || index >= NUM_ALARM || ppalarm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    alarm_init();
    *ppalarm = &s_alarms[index];
    return ESP_OK;
}

esp_err_t alarm_get_alarms(const struct alarm **palarms, int *num_alarm)
{
    if (palarms == NULL || num_alarm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    alarm_init();
    *palarms = s_alarms;
    *num_alarm = NUM_ALARM;
    return ESP_OK;
}

esp_err_t alarm_set_alarm(int index, const struct alarm *alarm)
{
    struct alarm *palarm;
    if (index < 0 || index >= NUM_ALARM || alarm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    alarm_init();
    palarm = &s_alarms[index];
    if (
        palarm->enabled == alarm->enabled &&
        strcmp(palarm->name, alarm->name) == 0 &&
        palarm->weeks == alarm->weeks &&
        palarm->seconds == alarm->seconds &&
        palarm->alarm_id == alarm->alarm_id &&
        1
    ) {
        return ESP_OK;
    }
    *palarm = *alarm;
    return alarm_save_alarm(index, palarm, NULL);
}

int64_t alarm_wakeup_us(struct tm *tm, suseconds_t *usec)
{
    int64_t wakeup_us = INT64_MAX;
    int sec = tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
    int i;
    alarm_init();
    for (i = 0; i < NUM_ALARM; i++) {
        struct alarm *palarm = &s_alarms[i];
        if (palarm->enabled) {
            int wday = tm->tm_wday;
            int64_t us = (palarm->seconds - sec)*1000000LL - *usec;
            if (us < 0) {
                wday = (wday+1)%7;
                us += 24*3600*1000000LL;
            }
            if ((palarm->weeks & (1<<wday)) && wakeup_us > us) {
                wakeup_us = us;
            }
        }
    }
    return wakeup_us;
}

bool alarm_get_current_alarm(struct tm *tm, int range, const struct alarm **ppalarm)
{
    int sec = tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
    int i;
    alarm_init();
    for (i = 0; i < NUM_ALARM; i++) {
        struct alarm *palarm = &s_alarms[i];
        int diff = palarm->seconds - sec;
        if (palarm->enabled) {
            if ((palarm->weeks & (1<<tm->tm_wday)) && diff <= 0 && diff >= -range) {
                *ppalarm = palarm;
                return true;
            }
        }
    }
    return false;
}
