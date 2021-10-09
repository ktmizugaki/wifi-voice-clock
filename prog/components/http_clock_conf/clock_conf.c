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
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include <esp_log.h>

#include "clock_conf.h"

#define TAG "clock_conf"
#define NVSKEY "clock"

#define SYNC_WEEKS      0x02    /**< bits of day of week to sync clock */
#define SYNC_HOUR       0       /**< hour to sync clock */
#define SYNC_MINUTE     0       /**< minute to sync clock */

static clock_conf_t s_clock_conf = {
    .TZ = "JST-9",
    .sync_weeks = SYNC_WEEKS,
    .sync_time = SYNC_HOUR*3600 + SYNC_MINUTE*60,
};

static esp_err_t clock_conf_load(clock_conf_t *conf)
{
    esp_err_t err, err2;
    nvs_handle_t nvsh;

    err = nvs_open(NVSKEY, NVS_READONLY, &nvsh);
    if (err != ESP_OK) {
        return err;
    }
    err = ESP_OK;
    err2 = nvs_get_u64(nvsh, "TZ", (uint64_t*)conf->TZ);
    conf->TZ[7] = 0;
    if (err2 != ESP_OK) {
        err = err2;
    }
    err2 = nvs_get_u8(nvsh, "weeks", &conf->sync_weeks);
    if (err2 != ESP_OK) {
        err = err2;
    }
    err2 = nvs_get_i32(nvsh, "time", &conf->sync_time);
    if (err2 != ESP_OK) {
        err = err2;
    }
    nvs_close(nvsh);
    return err;
}

static esp_err_t clock_conf_save(const clock_conf_t *conf)
{
    esp_err_t err, err2;
    nvs_handle_t nvsh;

    err = nvs_open(NVSKEY, NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }
    err = ESP_OK;
    err2 = nvs_set_u64(nvsh, "TZ", *(uint64_t*)conf->TZ);
    if (err2 != ESP_OK) {
        err = err2;
    }
    err2 = nvs_set_u8(nvsh, "weeks", conf->sync_weeks);
    if (err2 != ESP_OK) {
        err = err2;
    }
    err2 = nvs_set_i32(nvsh, "time", conf->sync_time);
    if (err2 != ESP_OK) {
        err = err2;
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvsh);
    }
    nvs_close(nvsh);
    return err;
}

esp_err_t clock_conf_init(void)
{
    clock_conf_load(&s_clock_conf);
    setenv("TZ", s_clock_conf.TZ, 1);
    tzset();

    return ESP_OK;
}

void clock_conf_get(clock_conf_t *conf)
{
    clock_conf_init();
    *conf = s_clock_conf;
}

esp_err_t clock_conf_set(const clock_conf_t *conf)
{
    esp_err_t err;
    err = clock_conf_save(&s_clock_conf);
    if (err == ESP_OK) {
        s_clock_conf = *conf;
    }
    return err;
}

int64_t clock_conf_wakeup_us(struct tm *tm, suseconds_t *usec)
{
    int wday = tm->tm_wday;
    int sec = tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
    int64_t diff = (s_clock_conf.sync_time - sec)*1000000LL - *usec;
    if (diff < 0) {
        wday = (wday+1)%7;
        diff += 24*3600*1000000LL;
    }
    if (!(s_clock_conf.sync_weeks & (1<<wday))) {
        ESP_LOGD(TAG, "not a day when to sync");
        return INT64_MAX;
    }
    ESP_LOGD(TAG, "wake up in %d sec", (int)(diff/1000000LL));
    return diff;
}

bool clock_conf_is_sync_time(struct tm *tm, int range)
{
    int sec = tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec;
    int diff = s_clock_conf.sync_time - sec;
    return (s_clock_conf.sync_weeks & (1<<tm->tm_wday)) && diff <= 0 && diff >= -range;
}
