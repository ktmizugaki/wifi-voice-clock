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

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* from simple_wifi/simple_sta.c */
#ifdef CONFIG_SWIFI_MAX_AP_CONFS
#define SWIFI_MAX_AP_CONFS  CONFIG_SWIFI_MAX_AP_CONFS
#else
#define SWIFI_MAX_AP_CONFS  3
#endif

struct wifi_conf {
    struct simple_wifi_ap_static_conf conf;
    char ntp[WIFI_CONF_NTP_SERVER_LENGTH];
};

#define WIFI_CONF_UPDATED               0
#define WIFI_CONF_ADDED                 1
#define WIFI_CONF_REMOVED               2
#define WIFI_CONF_NOT_FOUND             0x101
#define WIFI_CONF_TOO_MANY_ENTRY        0x102
#define WIFI_CONF_MISSING_PARAMS        0x103

extern esp_err_t wifi_conf_load(void);
extern esp_err_t wifi_conf_save(void);
extern int wifi_conf_get_count(void);
extern const struct wifi_conf* wifi_conf_get(int index);
extern const struct wifi_conf* wifi_conf_find(const char *ssid);
extern uint32_t wifi_conf_set(struct wifi_conf *conf);
extern uint32_t wifi_conf_remove(const char *ssid);

#ifdef __cplusplus
}
#endif
