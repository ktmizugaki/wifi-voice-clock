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

#include <time.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>
#include <audio.h>
#include <riffwave.h>

#include "storage.h"
#include "sound.h"
#include "voice.h"

#define TAG "voice"

#define TIMEVO_FILE     CONFIG_STORAGE_BASE_PATH "/time_vo.bin"

#define TIMEVO_MAX_NAME     15
#define TIMEVO_MAX_LEN       8

#define ERR_UNSET   -2

struct timevo {
    uint8_t indices[TIMEVO_MAX_LEN];
};

struct timevo_name {
    char name[TIMEVO_MAX_NAME+1];
};

static esp_err_t s_init_err = ERR_UNSET;
static struct timevo s_hours[24];
static struct timevo s_mins[60];
static struct timevo_name s_names[TIMEVO_MAX_LEN*2];

static esp_err_t load_timevo_indices(void)
{
    FILE *fp;
    int rsize;

    fp = fopen(TIMEVO_FILE, "r");
    if (fp == NULL) {
        ESP_LOGD(TAG, "failed to open time_vo.bin");
        return ESP_ERR_NOT_FOUND;
    }
    rsize = fread(s_hours, 1, sizeof(s_hours), fp);
    if (rsize != sizeof(s_hours)) {
        ESP_LOGD(TAG, "time_vo: Not enough hours");
        return ESP_ERR_INVALID_SIZE;
    }
    rsize = fread(s_mins, 1, sizeof(s_mins), fp);
    if (rsize != sizeof(s_mins)) {
        ESP_LOGD(TAG, "time_vo: Not enough min");
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

static int load_timevo_names(const struct timevo *timevo, struct timevo_name *names)
{
    FILE *fp;
    int i;

    if (s_init_err != ESP_OK) {
        return -1;
    }
    fp = fopen(TIMEVO_FILE, "r");
    if (fp == NULL) {
        return -1;
    }
    for (i = 0; i < TIMEVO_MAX_LEN && timevo->indices[i] != 0xff; i++) {
        int offset = TIMEVO_MAX_LEN*(24+60) + (TIMEVO_MAX_NAME+1) * timevo->indices[i];
        int rsize;
        if (fseek(fp, offset, SEEK_SET) < 0) {
            continue;
        }
        rsize = fread(names, 1, sizeof(*names), fp);
        if (rsize != sizeof(*names)) {
            continue;
        }
        ESP_LOGD(TAG, "loaded %d: %s", timevo->indices[i], names->name);
        names->name[TIMEVO_MAX_NAME] = '\0';
        names++;
    }
    fclose(fp);
    return i;
}

esp_err_t voice_ensure_init(void)
{
    esp_err_t err;
    err = sound_ensure_init();
    if (err != ESP_OK) {
        return err;
    }
    if (s_init_err == ERR_UNSET) {
        err = load_timevo_indices();
        if (err != ESP_OK) {
            s_init_err = err;
            return err;
        }
        s_init_err = ESP_OK;
    } else if (s_init_err != ESP_OK) {
        return s_init_err;
    }
    return ESP_OK;
}

esp_err_t voice_saytime(time_t time)
{
    struct tm tm;
    int count = 0;
    int i;
    esp_err_t err;

    err = voice_ensure_init();
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    localtime_r(&time, &tm);
    i = load_timevo_names(&s_hours[tm.tm_hour], s_names+count);
    ESP_LOGD(TAG, "hour%02d: %d names", tm.tm_hour, i);
    if (i < 0) {
        return ESP_FAIL;
    }
    count += i;
    i = load_timevo_names(&s_mins[tm.tm_min], s_names+count);
    ESP_LOGD(TAG, "min%02d: %d names", tm.tm_min, i);
    if (i < 0) {
        return ESP_FAIL;
    }
    count += i;
    for (i = 0; i < count; i++) {
        err = sound_play(s_names[i].name);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t voice_saynow(void)
{
    return voice_saytime(clock_time(NULL)+3);
}
