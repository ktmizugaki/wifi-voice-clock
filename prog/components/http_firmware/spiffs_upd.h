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

#ifndef SPIFFS_UPD_H
#define SPIFFS_UPD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_partition.h"
#include "esp_image_format.h"
#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque handle for an application OTA update
 *
 * spiffs_upd_begin() returns a handle which is then used for subsequent
 * calls to spiffs_upd_write() and spiffs_upd_end().
 */
typedef uint32_t spiffs_upd_handle_t;

/**
 * @brief   Commence an OTA update writing to the specified partition.

 * The specified partition is erased to the specified image size.
 *
 * If image size is not yet known, pass OTA_SIZE_UNKNOWN which will
 * cause the entire partition to be erased.
 *
 * On success, this function allocates memory that remains in use
 * until spiffs_upd_end() is called with the returned handle.
 *
 * @param partition Pointer to info for partition which will receive the OTA update. Required.
 * @param image_size Size of new spiffs image. Partition will be erased in order to receive this size of image. If 0 or OTA_SIZE_UNKNOWN, the entire partition is erased.
 * @param out_handle On success, returns a handle which should be used for subsequent spiffs_upd_write() and spiffs_upd_end() calls.

 * @return
 *    - ESP_OK: OTA operation commenced successfully.
 *    - ESP_ERR_INVALID_ARG: partition or out_handle arguments were NULL, or partition doesn't point to an spiffs partition.
 *    - ESP_ERR_NO_MEM: Cannot allocate memory for OTA operation.
 *    - ESP_ERR_NOT_FOUND: Partition argument not found in partition table.
 *    - ESP_ERR_INVALID_SIZE: Partition doesn't fit in configured flash size.
 *    - ESP_ERR_FLASH_OP_TIMEOUT or ESP_ERR_FLASH_OP_FAIL: Flash write failed.
 */
esp_err_t spiffs_upd_begin(const esp_partition_t* partition, size_t image_size, spiffs_upd_handle_t* out_handle);

/**
 * @brief   Write OTA update data to partition
 *
 * This function can be called multiple times as
 * data is received during the OTA operation. Data is written
 * sequentially to the partition.
 *
 * @param handle  Handle obtained from spiffs_upd_begin
 * @param data    Data buffer to write
 * @param size    Size of data buffer in bytes.
 *
 * @return
 *    - ESP_OK: Data was written to flash successfully.
 *    - ESP_ERR_INVALID_ARG: handle is invalid.
 *    - ESP_ERR_OTA_VALIDATE_FAILED: image contains invalid spiffs image magic bytes.
 *    - ESP_ERR_FLASH_OP_TIMEOUT or ESP_ERR_FLASH_OP_FAIL: Flash write failed.
 */
esp_err_t spiffs_upd_write(spiffs_upd_handle_t handle, const void* data, size_t size);

/**
 * @brief Finish OTA update and validate newly written spiffs image.
 *
 * @param handle  Handle obtained from spiffs_upd_begin().
 *
 * @note After calling spiffs_upd_end(), the handle is no longer valid and any memory associated with it is freed (regardless of result).
 *
 * @return
 *    - ESP_OK: Newly written OTA app image is valid.
 *    - ESP_ERR_NOT_FOUND: OTA handle was not found.
 *    - ESP_ERR_INVALID_ARG: Handle was never written to.
 *    - ESP_ERR_OTA_VALIDATE_FAILED: OTA image is invalid (either not a valid app image, or - if secure boot is enabled - signature failed to verify.)
 *    - ESP_ERR_INVALID_STATE: If flash encryption is enabled, this result indicates an internal error writing the final encrypted bytes to flash.
 */
esp_err_t spiffs_upd_end(spiffs_upd_handle_t handle);


/**
 * @brief Return the spiffs partition which should be written with a new spiffs image.
 *
 * Call this function to find an spiffs partition which can be passed to spiffs_upd_begin().
 *
 * Finds next partition round-robin, starting from the current running partition.
 *
 * @param label If set, partition of this label is searched. Can be NULL, in which case first spiffs partition will be returned.
 *
 * @return Pointer to info for spiffs partition. NULL result indicates spiffs partition is not found.
 *
 */
const esp_partition_t* spiffs_upd_find_partition(const char *label);

#ifdef __cplusplus
}
#endif

#endif /* SPIFFS_UPD_H */
