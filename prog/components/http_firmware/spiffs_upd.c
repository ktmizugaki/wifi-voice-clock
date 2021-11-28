// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
// Copyright 2021 Kawashima Teruaki
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <esp_err.h>
#include <esp_partition.h>
#include <esp_spiffs.h>
#include <esp_log.h>

#include "spiffs_upd.h"

#define TAG "spiffs_upd"

typedef struct spiffs_ops_entry {
    uint32_t handle;
    const esp_partition_t *part;
    uint32_t erased_size;
    uint32_t wrote_size;
} spiffs_ops_entry_t;

static spiffs_ops_entry_t *s_spiffs_ops_entry = NULL;

static uint32_t s_last_handle = 0;

/* Return true if this is an OTA app partition */
static bool is_spiffs_partition(const esp_partition_t *p)
{
    return (p != NULL
            && p->type == ESP_PARTITION_TYPE_DATA
            && p->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS);
}

esp_err_t spiffs_upd_verify(const esp_partition_t *p)
{
    if (!is_spiffs_partition(p)) {
        return ESP_ERR_INVALID_ARG;
    }
    /* TODO: how can I verify SPIFFS partition? */
    return ESP_OK;
}

esp_err_t spiffs_upd_begin(const esp_partition_t *partition, size_t image_size, spiffs_upd_handle_t *out_handle)
{
    uint32_t erased_size;
    esp_err_t ret;

    if ((partition == NULL) || (out_handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_spiffs_ops_entry != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    partition = esp_partition_verify(partition);
    if (partition == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    if (!is_spiffs_partition(partition)) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((image_size == 0) || (image_size == OTA_SIZE_UNKNOWN)) {
        erased_size = partition->size;
    } else {
        erased_size = ((image_size+SPI_FLASH_SEC_SIZE-1) / SPI_FLASH_SEC_SIZE) * SPI_FLASH_SEC_SIZE;
    }

    if (esp_spiffs_mounted(partition->label)) {
        esp_vfs_spiffs_unregister(partition->label);
    }
    ret = esp_partition_erase_range(partition, 0, erased_size);
    if (ret != ESP_OK) {
        return ret;
    }

    s_spiffs_ops_entry = malloc(sizeof(s_spiffs_ops_entry));
    if (s_spiffs_ops_entry == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_spiffs_ops_entry->erased_size = erased_size;

    s_spiffs_ops_entry->part = partition;
    s_spiffs_ops_entry->handle = ++s_last_handle;
    s_spiffs_ops_entry->wrote_size = 0;
    *out_handle = s_spiffs_ops_entry->handle;
    return ESP_OK;
}

esp_err_t spiffs_upd_write(spiffs_upd_handle_t handle, const void *data, size_t size)
{
    const uint8_t *data_bytes = (const uint8_t *)data;
    esp_err_t ret;
    spiffs_ops_entry_t *it;

    if (data == NULL) {
        ESP_LOGE(TAG, "write data is invalid");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_spiffs_ops_entry == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_spiffs_ops_entry->handle != handle) {
        return ESP_ERR_INVALID_ARG;
    }

    it = s_spiffs_ops_entry;
    // must erase the partition before writing to it
    assert(it->erased_size > 0 && "must erase the partition before writing to it");

    ret = esp_partition_write(it->part, it->wrote_size, data_bytes, size);
    if(ret == ESP_OK){
        it->wrote_size += size;
    }
    return ret;
}

esp_err_t spiffs_upd_end(spiffs_upd_handle_t handle)
{
    spiffs_ops_entry_t *it;
    esp_err_t ret = ESP_OK;

    if (s_spiffs_ops_entry == NULL || s_spiffs_ops_entry->handle != handle) {
        return ESP_ERR_INVALID_ARG;
    }


    /* 'it' holds the spiffs_ops_entry_t for 'handle' */
    it = s_spiffs_ops_entry;

    // spiffs_upd_end() is only valid if some data was written to this handle
    if ((it->erased_size == 0) || (it->wrote_size == 0)) {
        ret = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }

    if (spiffs_upd_verify(it->part) != ESP_OK) {
        ret = ESP_ERR_OTA_VALIDATE_FAILED;
        goto cleanup;
    }

 cleanup:
    s_spiffs_ops_entry->handle = 0;
    free(s_spiffs_ops_entry);
    return ret;
}

const esp_partition_t* spiffs_upd_find_partition(const char *label)
{
    const esp_partition_t *partition;
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, label);
    if (!is_spiffs_partition(partition)) {
        return NULL;
    }
    return partition;
}
