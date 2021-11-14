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

/**
 * @file
 * wrapper around esp_wifi.
 *
 * to connect ESP32 to access point,
 *  1. add access point configurations.
 *  2. call @ref simple_wifi_start with @ref SIMPLE_WIFI_MODE_STA included.
 *  3. scan access points.
 *  4. call @ref simple_wifi_connect function to one of available access point.
 *
 * alternatively, call @ref simple_wifi_connect_direct without scanning access points.
 *
 * scan and connect results are notified via @ref SIMPLE_WIFI_EVENT event.
 *
 * to make ESP32 a softap,
 *  1. call @ref simple_wifi_start with @ref SIMPLE_WIFI_MODE_SOFTAP included.
 *  2. use @ref simple_wifi_get_ssid and @ref simple_wifi_get_password for
 *     connection information.
 *
 * basically, SSID and password are generated automatically.
 * use these functions to get generated value after start wifi,
 */

/** maximum length of SSID including terminating null character. */
#define SWIFI_SSID_LEN  32
/** maximum length of password including terminating null character. */
#define SWIFI_PW_LEN    64
/** maximum length of ip address including terminating null character. */
#define SWIFI_IP_LEN    16

/** length of password when generating one internally. */
#ifdef CONFIG_SWIFI_SOFTAP_PW_LEN
#define SWIFI_SOFTAP_PW_LEN     CONFIG_SWIFI_SOFTAP_PW_LEN
#else
#define SWIFI_SOFTAP_PW_LEN     8 /* short to fit into small display */
#endif /* CONFIG_SWIFI_SOFTAP_PW_LEN */
/** minimum length of password. */
#define SWIFI_SOFTAP_MIN_PW_LEN 8

/** mode of wifi. */
enum simple_wifi_mode {
    SIMPLE_WIFI_MODE_STA = 1,           /**< wifi is station mode only */
    SIMPLE_WIFI_MODE_SOFTAP = 2,        /**< wifi is soft AP mode only */
    SIMPLE_WIFI_MODE_STA_SOFTAP = 3,    /**< wifi is both station and soft AP */
};

/** simple version of wifi_ap_record_t. */
struct simple_wifi_ap_info {
    char ssid[SWIFI_SSID_LEN];  /**< ssid of access point */
    int rssi;                   /**< signal strength of ap */
    int authmode;               /**< auhtentication mode of ap */
    int cipher_type;            /**< cipher type of ap */
    char ip[SWIFI_IP_LEN];      /**< ip address of sta if connected to ap */
};

/** configuration of an access point that ESP32 would connect to. */
struct simple_wifi_ap_conf {
    char ssid[SWIFI_SSID_LEN];      /**< ssid of access point */
    char password[SWIFI_PW_LEN];    /**< password of access point */
    bool use_static_ip;             /**< set to false to use DHCP */
};

/** extended simple_wifi_ap_conf to use static ip addres. */
struct simple_wifi_ap_static_conf {
    struct simple_wifi_ap_conf ap;  /**< base ap conf */
    char ip[SWIFI_IP_LEN];      /**< ip of this machine. used iff use_static_ip is true */
    char gateway[SWIFI_IP_LEN]; /**< ip of the gateway. used iff use_static_ip is true */
    char netmask[SWIFI_IP_LEN]; /**< netmask of the network. used iff use_static_ip is true */
};

/** state of scan process. */
enum simple_wifi_scan_state {
    SIMPLE_WIFI_SCAN_NONE,
    SIMPLE_WIFI_SCANNING,
    SIMPLE_WIFI_SCAN_DONE,
};

/** state of connection process. */
enum simple_wifi_connection_state {
    SIMPLE_WIFI_DISCONNECTED,
    SIMPLE_WIFI_CONNECTING,
    SIMPLE_WIFI_CONNECTED,
    SIMPLE_WIFI_DISCONNECTING,
};

/** initialize simple_wifi. */
extern esp_err_t simple_wifi_init(void);
/** start wifi station and/or ap. */
extern esp_err_t simple_wifi_start(enum simple_wifi_mode mode);
/** stop wifi started by @ref simple_wifi_start. */
extern void simple_wifi_stop(void);
/** return current wifi mode. return 0 if wifi is stopped. */
extern enum simple_wifi_mode simple_wifi_get_mode(void);

/** clear internal list of access points. */
extern void simple_wifi_clear_ap(void);
/** add specified access point to internal list of access points. */
extern esp_err_t simple_wifi_add_ap(const char *ssid, const char *password);
/** add specified access point to internal list of access points. */
extern esp_err_t simple_wifi_add_ap_static_ip(const char *ssid, const char *password, const char *ip, const char *gateway, const char *netmask);
/** add specified access point to internal list of access points. */
extern esp_err_t simple_wifi_add_ap_conf(const struct simple_wifi_ap_conf *conf, size_t conf_size);

/**
 * @brief start scanning access points.
 * send @ref SIMPLE_WIFI_EVENT_SCAN_DONE when done.
 */
extern esp_err_t simple_wifi_scan(void);
/**
 * @brief get list of scanned access points.
 * since returned ap points to internal buffer, @ref simple_wifi_release_scan_result
 * must be called with returned ap, as soon as possible after using result.
 * @return @ref simple_wifi_scan_state.
 */
extern enum simple_wifi_scan_state simple_wifi_get_scan_result(int *ap_num, const void **ap);
/** @brief release scan result returned by @ref simple_wifi_get_scan_result. */
extern void simple_wifi_release_scan_result(const void *ap);
/**
 * @brief connect to one of configured ap that mutches scanned ap.
 * send one of SIMPLE_WIFI_EVENT_STA_* in @ref simple_wifi_event_t when done.
 */
extern esp_err_t simple_wifi_connect(void);
/**
 * @brief connect to configured ap specified by ssid.
 * send one of SIMPLE_WIFI_EVENT_STA_* in @ref simple_wifi_event_t when done.
 */
extern esp_err_t simple_wifi_connect_direct(const char *ssid);
/** @brief disconnect from ap. */
extern void simple_wifi_disconnect(void);

/** @brief return true when scan result is available. */
extern bool simple_wifi_is_scan_result_available(void);
/** @brief return @ref simple_wifi_connection_state. */
extern enum simple_wifi_connection_state simple_wifi_get_connection_state(void);
/** @brief get info of connected ap if connected to ap. */
extern esp_err_t simple_wifi_get_connection_info(struct simple_wifi_ap_info *info);


/** @brief get SSID of softap. */
extern esp_err_t simple_wifi_get_ssid(char ssid[SWIFI_SSID_LEN]);
/** @brief set password to connect to softap. */
extern esp_err_t simple_wifi_set_password(const char *password);
/** @brief get password to connect to softap. */
extern esp_err_t simple_wifi_get_password(char password[SWIFI_PW_LEN]);

/** @brief helpfer function to get string representation of authmode. */
extern const char *simple_wifi_auth_mode_to_str(int authmode);
/** @brief helpfer function to get string representation of cipher_type. */
extern const char *simple_wifi_cipher_type_to_str(int cipher_type);

#ifdef __cplusplus
}
#endif
