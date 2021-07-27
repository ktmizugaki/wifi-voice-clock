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

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWIFI_SSID_LEN  32
#define SWIFI_PW_LEN    64
#define SWIFI_IP_LEN    16

#ifdef CONFIG_SWIFI_SOFTAP_PW_LEN
#define SWIFI_SOFTAP_PW_LEN     CONFIG_SWIFI_SOFTAP_PW_LEN
#else
#define SWIFI_SOFTAP_PW_LEN     8 /* short to fit into small display */
#endif /* CONFIG_SWIFI_SOFTAP_PW_LEN */
#define SWIFI_SOFTAP_MIN_PW_LEN 8

enum simple_wifi_mode {
    SIMPLE_WIFI_MODE_STA = 1,           /**< wifi is station mode only */
    SIMPLE_WIFI_MODE_SOFTAP = 2,        /**< wifi is soft AP mode only */
    SIMPLE_WIFI_MODE_STA_SOFTAP = 3,    /**< wifi is both station and soft AP */
};

struct simple_wifi_ap_info {
    char ssid[SWIFI_SSID_LEN];  /**< ssid of access point */
    int rssi;                   /**< signal strength of ap */
    int authmode;               /**< auhtentication mode of ap */
    int cipher_type;            /**< cipher type of ap */
    char ip[SWIFI_IP_LEN];      /**< ip address of sta if connected to ap */
};

struct simple_wifi_ap_conf {
    char ssid[SWIFI_SSID_LEN];      /**< ssid of access point */
    char password[SWIFI_PW_LEN];    /**< password of access point */
    bool use_static_ip;             /**< set to false to use DHCP */
};

struct simple_wifi_ap_static_conf {
    struct simple_wifi_ap_conf ap;
    char ip[SWIFI_IP_LEN];      /**< ip of this machine. used iff use_static_ip is true */
    char gateway[SWIFI_IP_LEN]; /**< ip of the gateway. used iff use_static_ip is true */
    char netmask[SWIFI_IP_LEN]; /**< netmask of the network. used iff use_static_ip is true */
};

enum simple_wifi_scan_state {
    SIMPLE_WIFI_SCAN_NONE,
    SIMPLE_WIFI_SCANNING,
    SIMPLE_WIFI_SCAN_DONE,
};

enum simple_wifi_connection_state {
    SIMPLE_WIFI_DISCONNECTED,
    SIMPLE_WIFI_CONNECTING,
    SIMPLE_WIFI_CONNECTED,
    SIMPLE_WIFI_DISCONNECTING,
};

extern esp_err_t simple_wifi_init(void);
extern esp_err_t simple_wifi_start(enum simple_wifi_mode mode);
extern void simple_wifi_stop(void);

/* configuring access points to be connected */
extern void simple_wifi_clear_ap(void);
extern esp_err_t simple_wifi_add_ap(const char *ssid, const char *password);
extern esp_err_t simple_wifi_add_ap_static_ip(const char *ssid, const char *password, const char *ip, const char *gateway, const char *netmask);
extern esp_err_t simple_wifi_add_ap_conf(const struct simple_wifi_ap_conf *conf, size_t conf_size);

/* manage esp32 station status */
extern esp_err_t simple_wifi_scan(void);
extern enum simple_wifi_scan_state simple_wifi_get_scan_result(int *ap_num, const void **ap);
extern void simple_wifi_release_scan_result(const void *ap);
extern esp_err_t simple_wifi_connect(void);
extern esp_err_t simple_wifi_connect_direct(const char *ssid);
extern void simple_wifi_disconnect(void);

extern bool simple_wifi_is_scan_result_available(void);
extern enum simple_wifi_connection_state simple_wifi_get_connection_state(void);
extern esp_err_t simple_wifi_get_connection_info(struct simple_wifi_ap_info *info);


/* configuring esp32 soft access point */
extern esp_err_t simple_wifi_get_ssid(char ssid[SWIFI_SSID_LEN]);
extern esp_err_t simple_wifi_set_password(const char *password);
extern esp_err_t simple_wifi_get_password(char password[SWIFI_PW_LEN]);

extern const char *simple_wifi_auth_mode_to_str(int authmode);
extern const char *simple_wifi_cipher_type_to_str(int cipher_type);

#ifdef __cplusplus
}
#endif
