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

#include <sys/stat.h>
#include <stdio.h>
#include <esp_spiffs.h>
#include <esp_err.h>
#include <esp_log.h>

#include <riffwave.h>

#include "storage.h"

#define TAG "storage"

esp_err_t storage_init(const char *base_path, const char *partition)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = base_path,
      .partition_label = partition,
      .max_files = 5,
      .format_if_mount_failed = false,
    };
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SPIFFS");

    if (base_path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (partition == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Use settings defined above to initialize and mount SPIFFS filesystem.
     * Note: esp_vfs_spiffs_register is an all-in-one convenience function. */
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    return ret;
}

esp_err_t storage_wav_open(const char *path, struct wav_play_info *info)
{
    char header[80];
    struct stat st;
    FILE *fp;
    int rsize;
    esp_err_t err;

    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    info->wav_data = NULL;

    if (stat(path, &st) != 0) {
        ESP_LOGE(TAG, "No file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Open error: %s", path);
        return ESP_FAIL;
    }

    rsize = fread(header, 1, sizeof(header), fp);
    if (rsize < MIN_RIFFWAVE_SIZE) {
        ESP_LOGE(TAG, "Read header error: %s: %d", path, rsize);
        fclose(fp);
        return ESP_FAIL;
    }
    err = audio_wav_parse(header, rsize, st.st_size, &info->wav_info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Parse header error: %s: %d", path, err);
        fclose(fp);
        return ESP_FAIL;
    }

    info->offset = 0;
    info->playsize = info->wav_info.data_length;
    info->data_func = storage_wav_copy_data_func;
    info->wav_data = fp;
    return ESP_OK;
}

esp_err_t storage_wav_read(struct wav_play_info *info, uint32_t offset, void *data, uint32_t *length)
{
    FILE *fp;
    uint32_t n, m;
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (info->wav_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (data == NULL || length == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (offset >= info->wav_info.data_length) {
        ESP_LOGE(TAG, "Offset exceeds data length: %u/%u", offset, info->wav_info.data_length);
        return ESP_ERR_INVALID_ARG;
    }
    fp = (FILE*)info->wav_data;
    n = *length;
    m = info->wav_info.data_length - offset;
    if (n > m) {
        n = m;
    }
    if (fseek(fp, info->wav_info.data_offset+offset, SEEK_SET) < 0) {
        return ESP_FAIL;
    }
    *length = fread(data, 1, n, fp);
    return ESP_OK;
}

void storage_wav_close(struct wav_play_info *info)
{
    FILE *fp = (FILE*)info->wav_data;
    if (fp != NULL) {
        fclose(fp);
        info->wav_data = NULL;
    }
}

int storage_wav_copy_data_func(struct wav_play_info *info, int offset, void *data, int size)
{
    uint32_t length = size;
    esp_err_t err;
    err = storage_wav_read(info, offset, data, &length);
    if (err != ESP_OK) {
        return -1;
    }
    return length;
}
