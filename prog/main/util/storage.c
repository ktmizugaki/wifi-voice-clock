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
