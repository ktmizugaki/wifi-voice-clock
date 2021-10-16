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

#include "audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * utilities to use riff wave (.wav file) in audio.
 */

#define MIN_RIFFWAVE_SIZE   44

struct wav_play_info;
/**
 * @brief user defined function to copy data from wav_data.
 * @param[in]  info     @ref wav_play_info containing wav_data,
 *                      passed to @ref audio_play or @ref audio_wav_play.
 * @param[in]  offset   offset of audio in wav_data. this value is relative to 'data' chunk data.
 * @param[out] data     pointer to buffer where the function should copy audio data to.
 * @param[in]  size     size of buffer pointed by data. user must copy exactly this size of data.
 * @return negative value to indicate error. 0 or positive value for success.
 */
typedef int (*wav_copy_data_func_t)(struct wav_play_info *info, int offset, void *data, int size);

/** structure to hold result of parsed wav file. */
struct wav_info {
    uint32_t samplerate;    /**< sampling rate of wave data */
    uint16_t channels;      /**< number of channels in wave data */
    uint16_t bits;          /**< number of bits of wave data */
    uint32_t data_offset;   /**< position where wave data starts */
    uint32_t data_length;   /**< lenth of wave data in bytes */
};

/** structure to hold info needed to play wave data. */
struct wav_play_info {
    struct wav_info wav_info;       /**< parsed wav info */
    uint32_t offset;                /**< current play position of wave audio data */
    int64_t playsize;               /**< remaining number of bytes to be played. play loops if it is greather than wav_info.data_length */
    wav_copy_data_func_t data_func; /**< copy data from wav_data */
    const void *wav_data;           /**< abstracted wave data to be used in data_func */
};

/**
 * @brief parse wav header in wav_data and store result to info.
 * wav_data can be partial as long as it is long enough to contains 'data'
 * chunk header.
 * @attention this function assumes wav_data contains only 'fmt ' and  'data'
 *            chunks in this order (after 'RIFF' XXXX 'WAVE') and does not support multiple 'data' chunks.
 * @param[in] wav_data      pointer to buffer contains wav header.
 * @param[in] wav_data_size size of wav_data.
 * @param[in] total_size    size of wav_data. this value is checked against 'data' chunk size.
 * @param[out] info         pointer to wav_info where parsed parameters will be stored.
 */
extern esp_err_t audio_wav_parse(const void *wav_data,
    uint32_t wav_data_size, uint32_t total_size,
    struct wav_info *info);

/**
 * @brief initialize @ref wav_play_info from wav data on memory.
 * the initialized @ref wav_play_info can be used for @ref audio_play along with
 * @ref audio_wav_data_func.
 * @note wav_data must point to whole wav on memory, unlike @ref audio_wav_parse.
 * @param[out] info         pointer to @ref wav_play_info which needs to be initialized.
 * @param[in] wav_data      pointer to buffer contains whole wav data.
 * @param[in] wav_data_size size of wav_data.
 */
extern esp_err_t audio_wav_play_init(struct wav_play_info *info,
    const void *wav_data, uint32_t wav_data_size);
/**
 * @brief helper function to start play wav_play_info.
 * @param[in] info          wav_play_info initialized with audio_wav_play_init.
 * @param[in] data_func     optinal data_func passed to audio_play.
 *                          audio_wav_data_func is used if not specified.
 */
extern void audio_wav_play(struct wav_play_info *info, audio_data_func_t data_func);
/**
 * callback function to read samples from wav data on memory. pass this
 * function to @ref audio_play with @ref wav_play_info initialized by @ref audio_wav_play_init.
 */
extern int audio_wav_data_func(void *arg, void *data, int *size);

/** calculate number of bytes for specified duration in msec using wav_info */
static inline int64_t wav_info_duration_to_bytes(int duration, const struct wav_info *info)
{
    return duration_to_bytes(duration, info->samplerate, info->channels, info->bits);
}

/** calculate duration in msec for specified number of bytes using wav_info */
static inline int wav_info_bytes_to_duration(int64_t bytes, const struct wav_info *info)
{
    return bytes_to_duration(bytes, info->samplerate, info->channels, info->bits);
}

#ifdef __cplusplus
}
#endif
