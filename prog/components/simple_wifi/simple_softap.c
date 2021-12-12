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
#include <nvs.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <esp_log.h>

#include "simple_wifi.h"
#include "simple_wifi_internal.h"

#define TAG "swifi_softap"

#ifdef CONFIG_SWIFI_SOFTAP_IP
#define SWIFI_SOFTAP_IP  CONFIG_SWIFI_SOFTAP_IP
#else
#define SWIFI_SOFTAP_IP  "192.168.4.1"
#endif

static const char PASSWORD_CHARS[] = {
    /* A-Z */
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52,
    0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
    /* a-z */
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A,
    /* 0-9 */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
};

static char s_ssid[SWIFI_SSID_LEN] = "";
static char s_password[SWIFI_PW_LEN] = "";

static void gen_password(char *password, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        password[i] = PASSWORD_CHARS[esp_random()%sizeof(PASSWORD_CHARS)];
    }
    password[len] = 0;
}

static esp_err_t save_password(const char *password, nvs_handle_t nvsh)
{
    esp_err_t err;
    err = nvs_set_str(nvsh, "pass", s_password);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_commit(nvsh);
    return err;
}

static esp_err_t load_password(char *password, size_t length, nvs_handle_t nvsh)
{
    esp_err_t err;
    size_t savedlength = length;
    err = nvs_get_str(nvsh, "pass", password, &savedlength);
    if (err != ESP_OK) {
        return err;
    }
    if (savedlength < SWIFI_SOFTAP_MIN_PW_LEN || savedlength > length) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    return ESP_OK;
}

esp_err_t simple_wifi_get_ssid(char ssid[SWIFI_SSID_LEN])
{
    uint8_t mac[6];
    if (s_ssid[0] == 0) {
        ESP_ERROR_CHECK( esp_wifi_get_mac(WIFI_IF_AP, mac) );
        snprintf(s_ssid, sizeof(s_ssid),
            SWIFI_IFNAME_PREFIX "-%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    strcpy(ssid, s_ssid);
    return ESP_OK;
}

esp_err_t simple_wifi_set_password(const char *password)
{
    esp_err_t err;
    nvs_handle_t nvsh;
    size_t length;

    length = strlen(password);
    if (length < SWIFI_SOFTAP_MIN_PW_LEN || length+1 >= SWIFI_PW_LEN) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strcmp(s_password, password) == 0) {
        return ESP_OK;
    }

    err = nvs_open("swifi", NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }
    err = save_password(password, nvsh);

    nvs_close(nvsh);
    if (err == ESP_OK) {
        strcpy(s_password, password);
    }
    return ESP_OK;
}

esp_err_t simple_wifi_get_password(char password[SWIFI_PW_LEN])
{
    esp_err_t err;
    nvs_handle_t nvsh;

    err = nvs_open("swifi", NVS_READWRITE, &nvsh);
    if (err != ESP_OK) {
        return err;
    }
    err = load_password(s_password, sizeof(s_password), nvsh);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        gen_password(s_password, SWIFI_SOFTAP_PW_LEN);
        err = save_password(s_password, nvsh);
    }
    strcpy(password, s_password);
    nvs_close(nvsh);
    return err;
}

esp_err_t simple_softap_configure(void)
{
    esp_err_t err;
    tcpip_adapter_ip_info_t ip_info;

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,    /* ssid is NULL terminated */
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 1,
        }
    };

    err = simple_wifi_get_ssid((char*)wifi_config.ap.ssid);
    if (err != ESP_OK) {
        return err;
    }
    err = simple_wifi_get_password((char*)wifi_config.ap.password);
    if (err != ESP_OK) {
        return err;
    }

    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );

    ip4addr_aton(SWIFI_SOFTAP_IP, &ip_info.ip);
    ip4addr_aton(SWIFI_SOFTAP_IP, &ip_info.gw);
    ip4addr_aton("255.255.255.0", &ip_info.netmask);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info);

    return ESP_OK;
}

void simple_softap_stop(void)
{
    /* nothing to do? */
}
