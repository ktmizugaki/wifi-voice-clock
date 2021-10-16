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

#include <stdint.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>

#include "audio.h"
#include "riffwave.h"

#define TAG "audio"

const uint32_t FCC_RIFF = 0x46464952;
const uint32_t FCC_WAVE = 0x45564157;
const uint32_t FCC_FMT = 0x20746d66;
const uint32_t FCC_DATA = 0x61746164;

struct wave_header {
    uint32_t riff;
    uint32_t riff_size;
    uint32_t wave;
    uint32_t fmt;
    uint32_t fmt_size;
    uint16_t fmt_tag;
    uint16_t fmt_channels;
    uint32_t fmt_samplerate;
    uint32_t fmt_bytes_per_sec;
    uint16_t fmt_block;
    uint16_t fmt_bps;
};

struct wave_data {
    uint32_t data;
    uint32_t data_size;
};

esp_err_t audio_wav_parse(const void *wav_data,
    uint32_t wav_data_size, uint32_t total_size,
    struct wav_info *info)
{
    struct wave_header header;
    struct wave_data data;
    uint32_t data_offset;

    if (wav_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (wav_data_size < 44) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&header, wav_data, sizeof(header));
    if (header.riff != FCC_RIFF || header.wave != FCC_WAVE || header.fmt != FCC_FMT) {
        ESP_LOGE(TAG, "Invalid header: riff=%08x wave=%08x fmt=%08x",
            header.riff, header.wave, header.fmt);
        return ESP_FAIL;
    }
    if (8+header.riff_size > total_size || 20+header.fmt_size+8 > wav_data_size) {
        ESP_LOGE(TAG, "Invalid size: riff=%d fmt=%d size=%d",
            header.riff_size, header.fmt_size, total_size);
        return ESP_FAIL;
    }
    if (header.fmt_tag != 1) {
        ESP_LOGE(TAG, "Unsupported wave format: %d", header.fmt_tag);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (header.fmt_samplerate != 48000 && header.fmt_samplerate != 44100 && header.fmt_samplerate != 16000 && header.fmt_samplerate != 8000) {
        ESP_LOGE(TAG, "Unsupported sample rate: %d", header.fmt_samplerate);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (header.fmt_channels != 1 && header.fmt_channels != 2) {
        ESP_LOGE(TAG, "Unsupported channels: %d", header.fmt_channels);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (header.fmt_bps != 8 && header.fmt_bps != 16) {
        ESP_LOGE(TAG, "Unsupported bits: %d", header.fmt_bps);
        return ESP_ERR_NOT_SUPPORTED;
    }
    data_offset = 20+header.fmt_size;
    memcpy(&data, (uint8_t*)wav_data+data_offset, sizeof(data));
    if (data.data != FCC_DATA) {
        ESP_LOGE(TAG, "Chunk after fmt is not data");
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (data_offset + 8 + data.data_size > total_size) {
        ESP_LOGE(TAG, "Invalid data size: data=%d, size=%d",
            data.data_size, total_size);
        return ESP_ERR_NOT_SUPPORTED;
    }

    info->samplerate = header.fmt_samplerate;
    info->channels = header.fmt_channels;
    info->bits = header.fmt_bps;
    info->data_offset = data_offset + 8;
    info->data_length = data.data_size;
    return ESP_OK;
}

static int audio_wav_copy_data_func(struct wav_play_info *info, int offset, void *data, int size)
{
    const uint8_t *wav_data = info->wav_data;
    memcpy(data, wav_data+offset, size);
    return size;
}

esp_err_t audio_wav_play_init(struct wav_play_info *info,
    const void *wav_data, uint32_t wav_data_size)
{
    esp_err_t err;
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    err = audio_wav_parse(wav_data, wav_data_size, wav_data_size, &info->wav_info);
    if (err != ESP_OK) {
        return err;
    }
    info->data_func = audio_wav_copy_data_func;
    info->wav_data = (const uint8_t*)wav_data+info->wav_info.data_offset;
    info->offset = 0;
    info->playsize = info->wav_info.data_length;
    return ESP_OK;
}

void audio_wav_play(struct wav_play_info *info, audio_data_func_t data_func)
{
    const struct wav_info *wav_info = &info->wav_info;
    audio_play(data_func? data_func: audio_wav_data_func, info,
        wav_info->samplerate, wav_info->channels, wav_info->bits,
        AUDIO_ENQUEUE);
}

int audio_wav_data_func(void *arg, void *data, int *size)
{
    struct wav_play_info *info = arg;
    wav_copy_data_func_t data_func;
    uint32_t length, remaining;
    int ret;

    if (data == NULL && size == NULL) {
        return 0;
    }

    info = arg;
    data_func = info->data_func? info->data_func: audio_wav_copy_data_func;
    length = *size;

    if (length > info->playsize) {
        length = info->playsize;
    }
    *size = 0;
    remaining = info->wav_info.data_length - info->offset;
    if (remaining == 0) {
        info->offset = 0;
    } else if (length > remaining) {
        ret = data_func(info, info->offset, data, remaining);
        if (ret < 0) {
            return 0;
        }
        *size += remaining;
        info->offset = 0;
        info->playsize -= remaining;
        data = (uint8_t*)data + remaining;
        length -= remaining;
    } else {
        remaining = 0;
    }
    ret = data_func(info, info->offset, data, length);
    if (ret < 0) {
        return 0;
    }
    *size = length + remaining;
    info->offset += length;
    info->playsize -= length;
    return info->playsize > 0;
}
