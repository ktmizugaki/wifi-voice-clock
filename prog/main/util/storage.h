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

#include <riffwave.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_STORAGE_BASE_PATH
#define CONFIG_STORAGE_BASE_PATH        "/spiffs"
#endif
#ifndef CONFIG_STORAGE_PARTITION_NAME
#define CONFIG_STORAGE_PARTITION_NAME   "storage"
#endif

extern esp_err_t storage_init(const char *base_path, const char *partition);
extern esp_err_t storage_wav_open(const char *path, struct wav_play_info *info);
extern esp_err_t storage_wav_read(struct wav_play_info *info, uint32_t offset, void *data, uint32_t *length);
extern void storage_wav_close(struct wav_play_info *info);
extern int storage_wav_copy_data_func(struct wav_play_info *info, int offset, void *data, int size);

#ifdef __cplusplus
}
#endif
