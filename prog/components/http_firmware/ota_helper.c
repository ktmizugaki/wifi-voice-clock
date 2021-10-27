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

#include <string.h>
#include <esp_ota_ops.h>
#include <esp_sleep.h>
#include <esp_log.h>

#include "ota_helper.h"

#define TAG "ota_helper"

static const char *OTA_STATE_NAMES[] = {
    [ESP_OTA_IMG_NEW            ] = "NEW",
    [ESP_OTA_IMG_PENDING_VERIFY ] = "VERIFY",
    [ESP_OTA_IMG_VALID          ] = "VALID",
    [ESP_OTA_IMG_INVALID        ] = "INVALID",
    [ESP_OTA_IMG_ABORTED        ] = "ABORTED",
};

void ota_print_partition_info(void)
{
    char subtype[10];
    const esp_partition_t *partition = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    esp_err_t err;

    if (partition->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        strcpy(subtype, "factory");
    } else if (
        partition->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
        partition->subtype < ESP_PARTITION_SUBTYPE_APP_OTA_MAX
    ) {
        snprintf(subtype, sizeof(subtype), "ota_%d", partition->subtype-ESP_PARTITION_SUBTYPE_APP_OTA_MIN);
    } else {
        snprintf(subtype, sizeof(subtype), "%02x", partition->subtype);
    }
    printf("current partition: type=%s, address=%#x, size=%#x, name=%.16s\n",
        subtype, partition->address, partition->size, partition->label);
    if (partition->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN
        && partition->subtype < ESP_PARTITION_SUBTYPE_APP_OTA_MAX)
    {
        err = esp_ota_get_state_partition(partition, &ota_state);
        if (err == ESP_OK) {
            if (ota_state >= 0 && ota_state < sizeof(OTA_STATE_NAMES)/sizeof(*OTA_STATE_NAMES)) {
                printf("    ota state: %s\n", OTA_STATE_NAMES[ota_state]);
            } else if (ota_state == ESP_OTA_IMG_UNDEFINED) {
                printf("    ota state: undefined\n");
            } else {
                printf("    ota state: unknown %d\n", ota_state);
            }
        }
    }
}

bool ota_need_self_test(void)
{
    const esp_partition_t *partition = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    esp_err_t err;

    err = esp_ota_get_state_partition(partition, &ota_state);
    return err == ESP_OK && (ota_state == ESP_OTA_IMG_PENDING_VERIFY || ota_state == ESP_OTA_IMG_UNDEFINED);
}

bool ota_can_rollback(void)
{
    return esp_ota_check_rollback_is_possible();
}

void ota_mark_valid(void)
{
    esp_err_t err;
    if (ota_need_self_test()) {
        err = esp_ota_mark_app_valid_cancel_rollback();
        if (err != ESP_OK) {
            ota_rollback();
        }
    }
}

void ota_rollback(void)
{
    if (ota_can_rollback()) {
        ESP_ERROR_CHECK(esp_ota_mark_app_invalid_rollback_and_reboot());
    }
    /* deep sleep forever when it is failed to rollback. */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
}
