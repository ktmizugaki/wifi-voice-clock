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
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

#include "simple_wifi.h"
#include "simple_wifi_event.h"
#include "simple_wifi_internal.h"

#define TAG "swifi_sta"

#ifdef CONFIG_SWIFI_MAX_AP_CONFS
#define SWIFI_MAX_AP_CONFS  CONFIG_SWIFI_MAX_AP_CONFS
#else
#define SWIFI_MAX_AP_CONFS  3
#endif

const wifi_auth_mode_t MIN_AUTH_MODE = WIFI_AUTH_WPA_PSK;
static int s_num_ap_conf = 0;
static struct simple_wifi_ap_static_conf s_ap_confs[SWIFI_MAX_AP_CONFS];

static int s_scan_expire_sec = 3*60;
static SemaphoreHandle_t s_scan_result_mutex = NULL;
static int s_last_scan = 0;
static int s_num_scan_result = 0;
static wifi_ap_record_t *s_scan_records = NULL;

enum simple_wifi_scan_state simple_scan_state = SIMPLE_WIFI_SCAN_NONE;
enum simple_wifi_connection_state simple_connection_state = SIMPLE_WIFI_DISCONNECTED;
int simple_sta_retry = 0;

static void expire_scan_result(void)
{
    int diff;
    if (xSemaphoreTake(s_scan_result_mutex, 3000/portTICK_PERIOD_MS) != pdTRUE) {
        return;
    }
    if (simple_scan_state == SIMPLE_WIFI_SCAN_DONE) {
        diff = swifi_time() - s_last_scan;
        if ((unsigned)diff >= s_scan_expire_sec) {
            simple_sta_clear_scan_result();
        }
    }
    xSemaphoreGive(s_scan_result_mutex);
}

esp_err_t simple_sta_init(void)
{
    if (s_scan_result_mutex == NULL) {
        s_scan_result_mutex = xSemaphoreCreateMutex();
        if (s_scan_result_mutex == NULL) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

void simple_wifi_clear_ap(void)
{
    simple_wifi__lock(portMAX_DELAY);
    s_num_ap_conf = 0;
    memset(&s_ap_confs, 0, sizeof(s_ap_confs));
    simple_wifi__unlock();
}

esp_err_t simple_wifi_add_ap(const char *ssid, const char *password)
{
    struct simple_wifi_ap_conf conf;
    if (strlen(ssid)+1 > SWIFI_SSID_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(password)+1 > SWIFI_PW_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    strcpy(conf.ssid, ssid);
    strcpy(conf.password, password);
    conf.use_static_ip = false;
    return simple_wifi_add_ap_conf(&conf, sizeof(conf));
}

esp_err_t simple_wifi_add_ap_static_ip(const char *ssid, const char *password, const char *ip, const char *gateway, const char *netmask)
{
    struct simple_wifi_ap_static_conf conf;
    if (strlen(ssid)+1 > SWIFI_SSID_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(password)+1 > SWIFI_PW_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    strcpy(conf.ap.ssid, ssid);
    strcpy(conf.ap.password, password);
    conf.ap.use_static_ip = true;
    strcpy(conf.ip, ip);
    strcpy(conf.gateway, gateway);
    strcpy(conf.netmask, netmask);
    return simple_wifi_add_ap_conf(&conf.ap, sizeof(conf));
}

esp_err_t simple_wifi_add_ap_conf(const struct simple_wifi_ap_conf *conf, size_t conf_size)
{
    if (conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (conf_size < sizeof(*conf) || conf_size > sizeof(s_ap_confs[0])) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!simple_wifi__lock(200/portTICK_PERIOD_MS)) {
        ESP_LOGW(TAG, "add_ap_conf: lock failed");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_num_ap_conf >= SWIFI_MAX_AP_CONFS) {
        simple_wifi__unlock();
        return ESP_ERR_NO_MEM;
    }
    memcpy(&s_ap_confs[s_num_ap_conf], conf, conf_size);
    s_num_ap_conf++;
    simple_wifi__unlock();
    return ESP_OK;
}

/* must be inside simple_wifi__lock when calling this function */
static bool find_ap_conf(const char *ssid, const struct simple_wifi_ap_conf **conf)
{
    int conf_index;
    for (conf_index = 0; conf_index < s_num_ap_conf; conf_index++) {
        *conf = &s_ap_confs[conf_index].ap;
        if (strcmp(ssid, (*conf)->ssid) == 0) {
            return true;
        }
    }
    return false;
}

/* s_scan_result_mutex must be locked before calling this function */
static bool find_ap_conf_from_scan(const struct simple_wifi_ap_conf **conf, const wifi_ap_record_t **ap)
{
    int ap_index;
    for (ap_index = 0; ap_index < s_num_scan_result; ap_index++) {
        const wifi_ap_record_t *record = &s_scan_records[ap_index];
        if (record->authmode < MIN_AUTH_MODE) {
            continue;
        }
        if (find_ap_conf((const char*)record->ssid, conf)) {
            ESP_LOGV(TAG, "Found matching ap %d: %s", ap_index, (const char *)(*conf)->ssid);
            *ap = record;
            return true;
        }
    }
    return false;
}

esp_err_t simple_sta_set_scan_result(void)
{
    esp_err_t ret;
    uint16_t ap_num;
    wifi_ap_record_t *records;
    if (xSemaphoreTake(s_scan_result_mutex, 3000/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGW(TAG, "set_scan_result: lock failed");
        return ESP_ERR_INVALID_STATE;
    }
    if (!(simple_wifi_mode & SIMPLE_WIFI_MODE_STA)) {
        xSemaphoreGive(s_scan_result_mutex);
        ESP_LOGD(TAG, "not in sta mode");
        return ESP_ERR_INVALID_STATE;
    }

    simple_sta_clear_scan_result();

    ret = esp_wifi_scan_get_ap_num(&ap_num);
    if (ret != ESP_OK) {
        xSemaphoreGive(s_scan_result_mutex);
        ESP_LOGW(TAG, "get_ap_num failed; %d, %d", ret, ap_num);
        return ESP_FAIL;
    }
    if (ap_num == 0) {
        ESP_LOGV(TAG, "no access points");
        s_last_scan = swifi_time();
        s_num_scan_result = 0;
        s_scan_records = NULL;
        simple_scan_state = SIMPLE_WIFI_SCAN_DONE;
        xSemaphoreGive(s_scan_result_mutex);
        swifi_event_post(SCAN_DONE, NULL, 0);
        return ESP_OK;
    }
    records = malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (records == NULL) {
        ESP_LOGW(TAG, "no memory");
        wifi_ap_record_t record;
        ap_num = 1;
        /* hope this releases allocated memory in wifi driver */
        esp_wifi_scan_get_ap_records(&ap_num, &record);
        simple_scan_state = SIMPLE_WIFI_SCAN_NONE;
        xSemaphoreGive(s_scan_result_mutex);
        return ESP_ERR_NO_MEM;
    }

    if (esp_wifi_scan_get_ap_records(&ap_num, records) != ESP_OK) {
        ESP_LOGW(TAG, "get_ap_records failed");
        free(records);
        xSemaphoreGive(s_scan_result_mutex);
        return ESP_FAIL;
    }
    s_last_scan = swifi_time();
    s_num_scan_result = ap_num;
    s_scan_records = records;
    simple_scan_state = SIMPLE_WIFI_SCAN_DONE;

    ESP_LOGV(TAG, "got %d ap records", ap_num);
    if (LOG_LOCAL_LEVEL >= ESP_LOG_VERBOSE) {
        uint16_t ap_index;
        for (ap_index = 0; ap_index < ap_num; ap_index++) {
            wifi_ap_record_t *record = &records[ap_index];
            ESP_LOGV(TAG, "%d: %s, %02x:%02x:%02x:%02x:%02x:%02x, %d, %d",
                ap_index, record->ssid,
                record->bssid[0],
                record->bssid[1],
                record->bssid[2],
                record->bssid[3],
                record->bssid[4],
                record->bssid[5],
                record->authmode, record->rssi);
        }
    }

    xSemaphoreGive(s_scan_result_mutex);
    swifi_event_post(SCAN_DONE, NULL, 0);
    return ESP_OK;
}

/* s_scan_result_mutex must be locked before calling this function */
void simple_sta_clear_scan_result(void)
{
    simple_scan_state = SIMPLE_WIFI_SCAN_NONE;
    s_num_scan_result = 0;
    if (s_scan_records) {
        free(s_scan_records);
        s_scan_records = NULL;
    }
}

esp_err_t simple_wifi_scan(void)
{
    if (!(simple_wifi_mode & SIMPLE_WIFI_MODE_STA)) {
        ESP_LOGD(TAG, "not in sta mode");
        return ESP_ERR_INVALID_STATE;
    }
    simple_wifi__lock(portMAX_DELAY);
    if (simple_scan_state == SIMPLE_WIFI_SCANNING) {
        simple_wifi__unlock();
        ESP_LOGD(TAG, "already scan is in progress");
        return ESP_OK;
    }
    if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
        simple_wifi__unlock();
        ESP_LOGD(TAG, "wifi is not disconnected");
        return ESP_ERR_INVALID_STATE;
    }
    simple_scan_state = SIMPLE_WIFI_SCANNING;

    ESP_LOGV(TAG, "Start wifi scan");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0, /* scan all channel */
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_PASSIVE,
        .scan_time.passive = 100,
    };
    ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_config, false) );
    simple_wifi__unlock();

    return ESP_OK;
}

enum simple_wifi_scan_state simple_wifi_get_scan_result(int *ap_num, const void **ap)
{
    enum simple_wifi_scan_state state;
    if (xSemaphoreTake(s_scan_result_mutex, 3000/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGW(TAG, "simple_wifi_get_scan_result: lock failed");
        *ap_num = 0;
        *ap = NULL;
        return SIMPLE_WIFI_SCAN_NONE;
    }
    state = simple_scan_state;
    if (simple_scan_state == SIMPLE_WIFI_SCAN_DONE && s_scan_records != NULL) {
        *ap_num = s_num_scan_result;
        *(wifi_ap_record_t**)ap = s_scan_records;
    } else {
        xSemaphoreGive(s_scan_result_mutex);
        *ap_num = 0;
        *ap = NULL;
    }
    return state;
}

void simple_wifi_release_scan_result(const void *ap)
{
    if (ap == NULL) {
        /* lock was not got */
        return;
    }
    if (ap != s_scan_records) {
        ESP_LOGE(TAG, "cannot release different result");
        return;
    }
    xSemaphoreGive(s_scan_result_mutex);
}

esp_err_t simple_wifi_connect(void)
{
    const struct simple_wifi_ap_static_conf *conf;
    const wifi_ap_record_t *ap;

    expire_scan_result();
    simple_wifi__lock(portMAX_DELAY);
    if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
        esp_err_t ret = simple_connection_state == SIMPLE_WIFI_DISCONNECTING? ESP_ERR_INVALID_STATE: ESP_OK;
        simple_wifi__unlock();
        ESP_LOGD(TAG, "state is not disconnected");
        return ret;
    }
    xSemaphoreTake(s_scan_result_mutex, portMAX_DELAY);
    if (!(simple_wifi_mode & SIMPLE_WIFI_MODE_STA)) {
        xSemaphoreGive(s_scan_result_mutex);
        simple_wifi__unlock();
        ESP_LOGD(TAG, "not in sta mode");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_num_ap_conf == 0) {
        xSemaphoreGive(s_scan_result_mutex);
        simple_wifi__unlock();
        ESP_LOGD(TAG, "no ap configured: %d, %d", s_num_scan_result, s_num_ap_conf);
        return ESP_ERR_INVALID_STATE;
    }

    if (!find_ap_conf_from_scan((const struct simple_wifi_ap_conf**)&conf, &ap)) {
        xSemaphoreGive(s_scan_result_mutex);
        simple_wifi__unlock();
        swifi_event_post(STA_FAIL, NULL, 0);
        ESP_LOGW(TAG, "No matching ap");
        return ESP_FAIL;
    }

    simple_connection_state = SIMPLE_WIFI_CONNECTING;
    simple_sta_retry = 5;
    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = 1,
            .listen_interval = 0,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = MIN_AUTH_MODE,
        }
    };
    strcpy((char*)wifi_config.sta.ssid, conf->ap.ssid);
    strcpy((char*)wifi_config.sta.password, conf->ap.password);
    memcpy(wifi_config.sta.bssid, ap->bssid, sizeof(wifi_config.sta.bssid));
    //wifi_config.sta.channel = ap->primary;
    xSemaphoreGive(s_scan_result_mutex);
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    if (conf->ap.use_static_ip) {
        tcpip_adapter_ip_info_t ip_info;
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
        ip4addr_aton(conf->ip, &ip_info.ip);
        ip4addr_aton(conf->gateway, &ip_info.gw);
        ip4addr_aton(conf->netmask, &ip_info.netmask);
        tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
    } else {
        tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
    }
    ESP_ERROR_CHECK( esp_wifi_connect() );
    simple_wifi__unlock();
    return ESP_OK;
}

esp_err_t simple_wifi_connect_direct(const char *ssid)
{
    const struct simple_wifi_ap_static_conf *conf;

    simple_wifi__lock(portMAX_DELAY);
    if (!(simple_wifi_mode & SIMPLE_WIFI_MODE_STA)) {
        simple_wifi__unlock();
        ESP_LOGD(TAG, "not in sta mode");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_num_ap_conf == 0) {
        simple_wifi__unlock();
        ESP_LOGD(TAG, "no ap configured: %d", s_num_ap_conf);
        return ESP_ERR_INVALID_STATE;
    }
    if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
        simple_wifi__unlock();
        ESP_LOGD(TAG, "state is not disconnected");
        return ESP_FAIL;
    }

    if (!find_ap_conf(ssid, (const struct simple_wifi_ap_conf**)&conf)) {
        simple_wifi__unlock();
        ESP_LOGW(TAG, "No matching ap");
        return ESP_FAIL;
    }

    simple_connection_state = SIMPLE_WIFI_CONNECTING;
    simple_sta_retry = 5;
    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = 0,
            .listen_interval = 0,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = MIN_AUTH_MODE,
        }
    };
    strcpy((char*)wifi_config.sta.ssid, conf->ap.ssid);
    strcpy((char*)wifi_config.sta.password, conf->ap.password);
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    if (conf->ap.use_static_ip) {
        tcpip_adapter_ip_info_t ip_info;
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
        ip4addr_aton(conf->ip, &ip_info.ip);
        ip4addr_aton(conf->gateway, &ip_info.gw);
        ip4addr_aton(conf->netmask, &ip_info.netmask);
        tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
    } else {
        tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
    }
    ESP_ERROR_CHECK( esp_wifi_connect() );
    simple_wifi__unlock();
    return ESP_OK;
}

void simple_wifi_disconnect(void)
{
    if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
        simple_wifi__lock(portMAX_DELAY);
        simple_sta_retry = 0;
        if (simple_connection_state == SIMPLE_WIFI_CONNECTING) {
            simple_connection_state = SIMPLE_WIFI_DISCONNECTED;
            esp_wifi_disconnect();
            swifi_event_post(STA_DISCONNECTED, NULL, 0);
        } else if (simple_connection_state == SIMPLE_WIFI_CONNECTED) {
            simple_connection_state = SIMPLE_WIFI_DISCONNECTING;
            esp_wifi_disconnect();
        }
        simple_wifi__unlock();
    }
}

enum simple_wifi_connection_state simple_wifi_get_connection_state(void)
{
    return simple_connection_state;
}

esp_err_t simple_wifi_get_connection_info(struct simple_wifi_ap_info *info)
{
    wifi_ap_record_t ap_info;
    tcpip_adapter_ip_info_t ip_info;
    if (simple_connection_state == SIMPLE_WIFI_CONNECTED &&
        esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
    {
        memcpy(info->ssid, ap_info.ssid, sizeof(info->ssid));
        info->rssi = ap_info.rssi;
        info->authmode = ap_info.authmode;
        info->cipher_type = ap_info.pairwise_cipher;
        if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info) == ESP_OK) {
            ip4addr_ntoa_r(&ip_info.ip, info->ip, sizeof(info->ip));
        } else {
            strcpy(info->ip, "0.0.0.0");
        }
        return ESP_OK;
    } else {
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
}

bool simple_wifi_is_scan_result_available(void)
{
    expire_scan_result();
    return simple_scan_state == SIMPLE_WIFI_SCAN_DONE;
}

void simple_sta_stop(void)
{
    simple_sta_retry = 0;
    if (simple_connection_state != SIMPLE_WIFI_DISCONNECTED) {
        simple_connection_state = SIMPLE_WIFI_DISCONNECTED;
        swifi_event_post(STA_DISCONNECTED, NULL, 0);
    }
    if (xSemaphoreTake(s_scan_result_mutex, 1000/portTICK_PERIOD_MS) == pdTRUE) {
        simple_sta_clear_scan_result();
        xSemaphoreGive(s_scan_result_mutex);
    }
    esp_wifi_disconnect();
}
