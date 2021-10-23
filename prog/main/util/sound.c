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

#define TAG "sound"

#define ERR_UNSET   -2

struct sound_play_info {
    struct wav_play_info base;
    char path[40];
    void (*notify_end_func)(void);
};

static esp_err_t s_init_err = ERR_UNSET;

static uint8_t s_sound_play_count = 0;

static int sound_play_file_func(void *arg, void *data, int *size)
{
    struct sound_play_info *play_info = (struct sound_play_info *)arg;
    struct wav_play_info *base = &play_info->base;

    if (data == NULL && size == NULL) {
        audio_wav_data_func(base, NULL, NULL);
        if (base->wav_data != NULL) {
            ESP_LOGD(TAG, "close %s", play_info->path);
            storage_wav_close(base);
        }
        if (play_info->notify_end_func != NULL) {
            play_info->notify_end_func();
        }
        free(play_info);
        s_sound_play_count--;
        return 0;
    }
    if (base->wav_data == NULL) {
        int64_t playsize = base->playsize;
        if (storage_wav_open(play_info->path, base) != ESP_OK) {
            *size = 0;
            return 0;
        }
        /* restore playsize reset by storage_wav_open */
        base->playsize = playsize;
    }

    return audio_wav_data_func(base, data, size);
}

esp_err_t sound_ensure_init(void)
{
    esp_err_t err;
    if (s_init_err == ERR_UNSET) {
        err = audio_init();
        if (err != ESP_OK) {
            s_init_err = err;
            return err;
        }
        err = storage_init(CONFIG_STORAGE_BASE_PATH, CONFIG_STORAGE_PARTITION_NAME);
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

esp_err_t sound_play_repeat_notify(const char *name, int duration, void (*notify_end_func)(void))
{
    struct sound_play_info *play_info;
    struct wav_play_info *base;
    struct wav_info *wav_info;
    esp_err_t err;

    if (name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_sound_play_count > 64) {
        ESP_LOGW(TAG, "queued too many sounds: %d", s_sound_play_count);
        return ESP_ERR_INVALID_STATE;
    }

    err = sound_ensure_init();
    if (err != ESP_OK) {
        return ESP_FAIL;
    }

    play_info = malloc(sizeof(struct sound_play_info));
    if (play_info == NULL) {
        return ESP_ERR_NO_MEM;
    }
    base = &play_info->base;

    strcpy(play_info->path, CONFIG_STORAGE_BASE_PATH "/");
    strlcat(play_info->path, name, sizeof(play_info->path));
    err = storage_wav_open(play_info->path, base);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wav open error: %s: %d", play_info->path, err);
        free(play_info);
        return ESP_FAIL;
    }
    /* close FILE to save FD */
    storage_wav_close(base);

    wav_info = &base->wav_info;
    base->offset = 0;
    if (duration < 0) {
        /* count to size */
        base->playsize = -(int64_t)duration*wav_info->data_length;
    } else {
        /* milliseconds to size */
        base->playsize = wav_info_duration_to_bytes(duration, wav_info);
    }
    play_info->notify_end_func = notify_end_func;
    ESP_LOGD(TAG, "wav opened: %s: %d bytes, %ld msec",
        play_info->path, wav_info->data_length,
        (long)wav_info_bytes_to_duration(wav_info->data_length, wav_info));
    s_sound_play_count++;
    audio_wav_play(base, sound_play_file_func);
    return ESP_OK;
}

esp_err_t sound_play_repeat(const char *name, int duration)
{
    return sound_play_repeat_notify(name, duration, NULL);
}

esp_err_t sound_play(const char *name)
{
    return sound_play_repeat(name, -1);
}

bool sound_is_playing(void)
{
    return s_sound_play_count > 0 || audio_is_playing();
}

void sound_stop(void)
{
    audio_stop();
    while (sound_is_playing()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
