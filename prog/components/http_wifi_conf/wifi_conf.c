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
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>
#include <esp_wifi_types.h>

#include <simple_wifi.h>
#include <simple_wifi_event.h>

#include "wifi_conf.h"
#include "internal.h"

#define TAG "wifi_conf"

struct wifi_conf_state {
    bool valid;
    bool changed;
};

static int8_t s_num_wifi_conf = -1;
static struct wifi_conf_state s_wifi_conf_states[SWIFI_MAX_AP_CONFS];
static struct wifi_conf s_wifi_confs[SWIFI_MAX_AP_CONFS];

static bool s_waiting_scan_done = false;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == SIMPLE_WIFI_EVENT) {
        switch(event_id) {
        case SIMPLE_WIFI_EVENT_SCAN_DONE:
            if (s_waiting_scan_done) {
                simple_wifi_connect();
                s_waiting_scan_done = false;
            }
            break;
        }
    }
}

esp_err_t wifi_conf_load(void)
{
    char name[8] = "conf00";
    esp_err_t err;
    nvs_handle_t nvsh;
    size_t length;
    int i, n;

    if (s_num_wifi_conf >= 0) {
        return ESP_OK;
    }

    err = nvs_open("wifi_conf", NVS_READONLY, &nvsh);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        s_num_wifi_conf = 0;
        for (i = 0, n = 0; i < SWIFI_MAX_AP_CONFS; i++) {
            s_wifi_conf_states[i].valid = false;
            s_wifi_conf_states[i].changed = false;
        }
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0, n = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        name[3] = '0'+(i/10);
        name[4] = '0'+(i%10);
        length = sizeof(struct wifi_conf);
        err = nvs_get_blob(nvsh, name, &s_wifi_confs[i], &length);
        if (err == ESP_OK && length == sizeof(struct wifi_conf)) {
            s_wifi_conf_states[i].valid = true;
            s_wifi_conf_states[i].changed = false;
            n++;
        } else {
            s_wifi_conf_states[i].valid = false;
            s_wifi_conf_states[i].changed = false;
        }
    }
    s_num_wifi_conf = n;
    nvs_close(nvsh);

    return err;
}

esp_err_t wifi_conf_save(void)
{
    char name[8] = "conf00";
    esp_err_t err;
    nvs_handle_t nvsh;
    int i;

    err = nvs_open("wifi_conf", NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        name[3] = '0'+(i/10);
        name[4] = '0'+(i%10);
        if (!s_wifi_conf_states[i].changed) {
            err = ESP_OK;
        } else if (s_wifi_conf_states[i].valid) {
            err = nvs_set_blob(nvsh, name, &s_wifi_confs[i], sizeof(struct wifi_conf));
        } else {
            err = nvs_erase_key(nvsh, name);
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                err = ESP_OK;
            }
        }
        if (err != ESP_OK) {
            goto end;
        }
        s_wifi_conf_states[i].changed = false;
    }

    err = nvs_commit(nvsh);

end:
    nvs_close(nvsh);
    return err;
}

static int find_wifi_conf(const char *ssid)
{
    int i;
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        if (s_wifi_conf_states[i].valid) {
            if (strcmp(s_wifi_confs[i].conf.ap.ssid, ssid) == 0) {
                return i;
            }
        }
    }
    return -1;
}

static int find_unsued_wifi_conf(void)
{
    int i;
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        if (!s_wifi_conf_states[i].valid) {
            return i;
        }
    }
    return -1;
}

int wifi_conf_get_count(void)
{
    return s_num_wifi_conf;
}

const struct wifi_conf* wifi_conf_get(int index)
{
    if (index < 0 || index >= SWIFI_MAX_AP_CONFS) {
        return NULL;
    }
    if (!s_wifi_conf_states[index].valid) {
        return NULL;
    }
    return &s_wifi_confs[index];
}

const struct wifi_conf* wifi_conf_find(const char *ssid)
{
    int conf_index;

    conf_index = find_wifi_conf(ssid);
    if (conf_index == -1) {
        return NULL;
    }
    return &s_wifi_confs[conf_index];
}

uint32_t wifi_conf_set(struct wifi_conf *conf)
{
    uint32_t ret;
    int conf_index;

    conf_index = find_wifi_conf(conf->conf.ap.ssid);
    if (conf_index == -1 && conf->conf.ap.password[0] == '\0') {
        return WIFI_CONF_MISSING_PARAMS;
    }
    if (conf->conf.ap.use_static_ip) {
        if (conf->conf.ip[0] == '\0' || conf->conf.gateway[0] == '\0') {
            return WIFI_CONF_MISSING_PARAMS;
        }
        if (conf->conf.netmask[0] == '\0') {
            strcpy(conf->conf.netmask, "255.255.255.0");
        }
    }

    if (conf_index == -1) {
        conf_index = find_unsued_wifi_conf();
        if (conf_index == -1) {
            return WIFI_CONF_TOO_MANY_ENTRY;
        }
        s_num_wifi_conf++;
        ret = WIFI_CONF_ADDED;
    } else {
        ret = WIFI_CONF_UPDATED;
    }
    if (conf->conf.ap.password[0] == '\0') {
        strcpy(conf->conf.ap.password, s_wifi_confs[conf_index].conf.ap.password);
    }
    memcpy(&s_wifi_confs[conf_index], conf, sizeof(struct wifi_conf));
    s_wifi_conf_states[conf_index].valid = true;
    s_wifi_conf_states[conf_index].changed = true;
    wifi_conf_save();
    return ret;
}

uint32_t wifi_conf_remove(const char *ssid)
{
    int conf_index;

    conf_index = find_wifi_conf(ssid);
    if (conf_index == -1) {
        return WIFI_CONF_NOT_FOUND;
    }

    s_num_wifi_conf--;
    s_wifi_conf_states[conf_index].valid = false;
    s_wifi_conf_states[conf_index].changed = true;
    wifi_conf_save();
    return WIFI_CONF_REMOVED;
}

bool wifi_conf_configured(void)
{
    wifi_conf_load();
    return s_num_wifi_conf != 0;
}

esp_err_t wifi_conf_connect(void)
{
    int i;
    int timeout;
    esp_err_t err;

    if (simple_wifi_get_connection_state() == SIMPLE_WIFI_CONNECTED) {
        return ESP_OK;
    }

    wifi_conf_load();

    if (s_num_wifi_conf == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    simple_wifi_disconnect();
    simple_wifi_clear_ap();
    for (i = 0; i < SWIFI_MAX_AP_CONFS; i++) {
        struct wifi_conf *conf = &s_wifi_confs[i];
        if (!s_wifi_conf_states[i].valid) {
            continue;
        }
        ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&conf->conf.ap, sizeof(conf->conf)) );
    }

    timeout = 30;
    while (simple_wifi_get_connection_state() == SIMPLE_WIFI_DISCONNECTING && --timeout > 0) {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    if (timeout == 0) {
        ESP_LOGW(TAG, "Failed to disconnect");
        return ESP_ERR_TIMEOUT;
    }

    if (!simple_wifi_is_scan_result_available()) {
        s_waiting_scan_done = true;
        /* overwrites previous registration if exists */
        ESP_ERROR_CHECK( esp_event_handler_register(
            SIMPLE_WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
        simple_wifi_scan();
        return ESP_OK;
    }

    err = simple_wifi_connect();
    return err == ESP_OK? ESP_OK: ESP_FAIL;
}

esp_err_t wifi_conf_connect_direct(const char *ssid)
{
    int conf_index;
    int timeout;
    esp_err_t err;

    wifi_conf_load();

    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_num_wifi_conf == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    conf_index = find_wifi_conf(ssid);
    if (conf_index == -1) {
        return ESP_ERR_NOT_FOUND;
    }

    simple_wifi_disconnect();
    simple_wifi_clear_ap();
    ESP_ERROR_CHECK( simple_wifi_add_ap_conf(&s_wifi_confs[conf_index].conf.ap, sizeof(s_wifi_confs[conf_index].conf)) );
    timeout = 30;
    while (simple_wifi_get_connection_state() == SIMPLE_WIFI_DISCONNECTING && --timeout > 0) {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    if (simple_wifi_get_connection_state() != SIMPLE_WIFI_DISCONNECTED) {
        return ESP_ERR_TIMEOUT;
    }

    err = simple_wifi_connect_direct(ssid);
    return err == ESP_OK? ESP_OK: ESP_FAIL;
}

esp_err_t wifi_conf_get_ntp(char ntp[WIFI_CONF_NTP_SERVER_LENGTH])
{
    struct simple_wifi_ap_info info;
    int conf_index;
    esp_err_t err;

    err = simple_wifi_get_connection_info(&info);
    if (err != ESP_OK) {
        return err;
    }

    conf_index = find_wifi_conf(info.ssid);
    if (conf_index == -1) {
        return ESP_ERR_NOT_FOUND;
    }
    strcpy(ntp, s_wifi_confs[conf_index].ntp);

    return ESP_OK;
}
