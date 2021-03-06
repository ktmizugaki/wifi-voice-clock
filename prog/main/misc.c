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
#include <stdint.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>

#include <clock.h>
#include <clock_conf.h>
#include <alarm.h>
#include <audio.h>
#include <riffwave.h>
#include <simple_wifi_event.h>
#include <lan_manager.h>
#if CONFIG_USE_SYSLOG
#include <vcc.h>
#include <udplog.h>
#endif /* CONFIG_USE_SYSLOG */
#include <gfx.h>

#include "app_event.h"
#include "app_clock.h"
#include "app_display.h"
#include "power.h"
#include "app_mode.h"
#include "sound.h"

#include "misc.h"

#define TAG "misc"

static time_t s_task_check_time = 0;
static uint8_t s_playing_alarm = 0;
static struct alarm s_alarm;

#if CONFIG_USE_SYSLOG
void misc_ensure_init_udplog(void)
{
    static bool initialized = false;
    ip4_addr_t addr;

    if (initialized) {
        return;
    }
    initialized = true;
    ip4addr_aton(CONFIG_SYSLOG_IP, &addr);
    udplog_init("voice-clock", LOG_LOCAL0, &addr, -1);
}

void misc_udplog_vcc(void)
{
    int vcc;

    if (!lan_manager_request_conn()) {
        return;
    }
    if (vcc_get_last_value(&vcc) != ESP_OK) {
        lan_manager_release_conn();
        return;
    }
    if (lan_manager_is_connected()) {
        misc_ensure_init_udplog();
        udplog(LOG_INFO, "vcc is %dmV", vcc);
    }
    lan_manager_release_conn();
}
#endif /* CONFIG_USE_SYSLOG */

void misc_ensure_vcc_level(vcc_level_t min_level, bool is_interactive)
{
    vcc_level_t level = vcc_get_level(false);
    if (level <= min_level) {
        return;
    }
    if (level <= VCC_LEVEL_WARNING) {
        if (is_interactive) {
            misc_beep(1000);
            if (app_display_ensure_init() == ESP_OK) {
                app_display_ensure_reset();
                app_display_clear();
                gfx_text_puts_xy(LCD, &gfx_tinyfont, "Low Battery...", 0, 0);
                app_display_update();
            }
            vTaskDelay(1500/portTICK_PERIOD_MS);
        }
    }
    if (level <= VCC_LEVEL_DECREASING2) {
        power_suspend();
    } else if (level <= VCC_LEVEL_WARNING) {
        power_hibernate();
    } else {
        power_halt();
    }
}

bool misc_process_time_task(void)
{
    time_t time;
    struct tm tm;
    int range;
    const struct alarm *palarm;
    bool result = false;
    range = (time = clock_time(NULL)) - s_task_check_time;
    if (s_task_check_time == 0 || range > 20 || range < -2) {
        range = 20;
    } else if (range < 0) {
        range = 0;
    }
    s_task_check_time = time;
    clock_localtime(&tm);
    if (clock_conf_is_sync_time(&tm, range)) {
        misc_ensure_vcc_level(VCC_LEVEL_WARNING, false);
        app_clock_start_sync();
        result = true;
    }
    if (!misc_is_playing_alarm() && alarm_get_current_alarm(&tm, range, &palarm)) {
        misc_ensure_vcc_level(VCC_LEVEL_CRITICAL, false);
        audio_stop();
        misc_play_alarm(palarm);
        result = true;
    }
    return result;
}

void misc_handle_event(const app_event_t *event)
{
    switch (event->id) {
    case APP_EVENT_CLOCK:
        misc_process_time_task();
        break;
    case APP_EVENT_SYNC:
        if (event->arg0 == APP_SYNC_FAIL || event->arg0 == APP_SYNC_SUCCESS) {
            app_clock_stop_sync();
        }
        break;
    case APP_EVENT_WIFI:
#if CONFIG_USE_SYSLOG
        if (event->arg0 == SIMPLE_WIFI_EVENT_STA_CONNECTED) {
            misc_udplog_vcc();
        }
#endif /* CONFIG_USE_SYSLOG */
        break;
    default:
        break;
    }
}

static void on_notify_end(void)
{
    s_playing_alarm--;
}

static int misc_play_data_func(void *arg, void *data, int *size)
{
    if (data == NULL && size == NULL) {
        audio_wav_data_func(arg, NULL, NULL);
        on_notify_end();
        free(arg);
        return 0;
    }
    return audio_wav_data_func(arg, data, size);
}

bool misc_is_playing_alarm(void)
{
    return s_playing_alarm > 0;
}

void misc_play_alarm(const struct alarm *alarm)
{
    char name[16];
    esp_err_t err;
    if (s_playing_alarm) {
        return;
    }
    s_alarm = *alarm;
    ESP_LOGI(TAG, "play alarm %s", s_alarm.name);
    snprintf(name, sizeof(name), "alarm%d.wav", s_alarm.alarm_id);
    err = sound_play_repeat_notify(name, 15*1000, on_notify_end);
    if (err != ESP_OK) {
        /* failed to play alarm in spiffs. fallback to wav embedded in program */
        misc_play_default_alarm();
    } else {
        s_playing_alarm++;
    }
}

void misc_play_default_alarm(void)
{
    extern const uint8_t alarm_start[] asm("_binary_alarm_wav_start");
    extern const uint8_t alarm_end[] asm("_binary_alarm_wav_end");
    const uint32_t alarm_size = alarm_end - alarm_start;
    struct wav_play_info *info;
    struct wav_info *wav_info;
    esp_err_t err;

    if (s_playing_alarm) {
        return;
    }
    err = audio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to init audio: %d", err);
        return;
    }
    info = malloc(sizeof(struct wav_play_info));
    if (info == NULL) {
        ESP_LOGE(TAG, "failed to allocate memory");
        return;
    }
    err = audio_wav_play_init(info, alarm_start, alarm_size);
    if (err != ESP_OK) {
        free(info);
        ESP_LOGE(TAG, "failed to parse default alarm.wav");
        return;
    }
    wav_info = &info->wav_info;
    info->offset = 0;
    info->playsize = wav_info_duration_to_bytes(15000, wav_info);
    audio_wav_play(info, misc_play_data_func);
    s_playing_alarm++;
}

esp_err_t misc_beep(int duration)
{
    esp_err_t err;
    err = audio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to init audio: %d", err);
        return err;
    }
    audio_beep(BEEP_FREQ, duration);
    return ESP_OK;
}
