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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_timer.h>
#include <esp_err.h>
#include <esp_log.h>

#include "vcc.h"

#define TAG "vcc"

#define NUMBER_OF_SAMPLES   64

static const adc2_channel_t ADC2_CHANNEL = (adc2_channel_t)CONFIG_VCC_ADC2_CHANNEL;
static const uint32_t DEFAULT_VREF = CONFIG_VCC_DEFAULT_VREF;
static const adc_atten_t ATTEN = ADC_ATTEN_DB_6;
static const gpio_num_t VCC_PIN = (gpio_num_t)CONFIG_VCC_PIN;
static const int64_t CACHE_LIFETIME = 60*1000000;

static esp_adc_cal_characteristics_t s_adc_chars;
static uint64_t s_last_measure_time = 0;
static int s_last_vcc = 0;
#if CONFIG_VCC_CHARGE_STAT_PIN >= 0
static const gpio_num_t CHARGE_STAT_PIN = (gpio_num_t)CONFIG_VCC_CHARGE_STAT_PIN;
#endif

static bool cache_is_valid(void)
{
    return s_last_measure_time != 0 &&
        (esp_timer_get_time() - s_last_measure_time) <= CACHE_LIFETIME;
}

esp_err_t vcc_init(void)
{
    esp_err_t err;
    esp_adc_cal_value_t val_type;
    gpio_config_t io_conf = {};

    val_type = esp_adc_cal_characterize(ADC_UNIT_2, ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, &s_adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGD(TAG, "Using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGD(TAG, "Using eFuse Vref");
    } else {
        ESP_LOGD(TAG, "Using Default Vref");
    }

    gpio_set_level(VCC_PIN, 0);
    io_conf.pin_bit_mask = BIT64(VCC_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    err = gpio_config(&io_conf);
    ESP_LOGD(TAG, "Config gpio %d: %d", VCC_PIN, err);
#if CONFIG_VCC_CHARGE_STAT_PIN >= 0
    io_conf.pin_bit_mask = BIT64(CHARGE_STAT_PIN);
    io_conf.mode = GPIO_MODE_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    err = gpio_config(&io_conf);
    ESP_LOGD(TAG, "Config gpio %d: %d", CHARGE_STAT_PIN, err);
#endif
    return ESP_OK;
}

esp_err_t vcc_read(int *vcc, bool nocache)
{
    esp_err_t err;
    int i, raw, sum;

    if (vcc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!nocache && cache_is_valid()) {
        *vcc = s_last_vcc;
        return ESP_OK;
    }

    *vcc = 0;
    err = adc2_config_channel_atten(ADC2_CHANNEL, ATTEN);
    if (err != ESP_OK) {
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGD(TAG, "wifi is active");
        }
        return err;
    }
    gpio_set_level(VCC_PIN, 1);
    adc_power_on();
    vTaskDelay(50/portTICK_PERIOD_MS);
    adc2_get_raw(ADC2_CHANNEL, ADC_WIDTH_BIT_12, &raw);
    for (i = 0, sum = 0; i < NUMBER_OF_SAMPLES; i++) {
        if (adc2_get_raw(ADC2_CHANNEL, ADC_WIDTH_BIT_12, &raw) != ESP_OK) {
            adc_power_off();
            gpio_set_level(VCC_PIN, 0);
            return ESP_FAIL;
        }
        sum += raw;
    }
    sum /= NUMBER_OF_SAMPLES;
    adc_power_off();
    gpio_set_level(VCC_PIN, 0);

    s_last_measure_time = esp_timer_get_time();
    *vcc = s_last_vcc = esp_adc_cal_raw_to_voltage(sum, &s_adc_chars) * 2;
    return ESP_OK;
}

esp_err_t vcc_get_last_value(int *vcc)
{
    if (vcc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_last_vcc == 0) {
        return ESP_FAIL;
    }
    *vcc = s_last_vcc;
    return ESP_OK;
}

vcc_level_t vcc_get_level(bool nocache)
{
    int vcc;
    if (vcc_read(&vcc, nocache) != ESP_OK) {
        return VCC_LEVEL_UNKNOWN;
    }
    if (vcc > 3230) {
        return VCC_LEVEL_OK;
    } else if (vcc > 3160) {
        return VCC_LEVEL_DECREASING;
    } else if (vcc > 3000) {
        return VCC_LEVEL_DECREASING2;
    } else if (vcc > 2900) {
        return VCC_LEVEL_WARNING;
    } else {
        return VCC_LEVEL_CRITICAL;
    }
}

#if CONFIG_VCC_CHARGE_STAT_PIN >= 0
/*
 * MCP73831:
 *   STAT=LOW  ... charging
 *   STAT=HIGH ... completed
 *   STAT=Hi-Z ... shutdown: Battery or Power is absent.
 */
esp_err_t vcc_get_charge_state(vcc_charge_state_t *state)
{
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *state = VCC_CHARG_DISCONNECTED;
    gpio_set_direction(CHARGE_STAT_PIN, GPIO_MODE_INPUT);

    gpio_set_pull_mode(CHARGE_STAT_PIN, GPIO_PULLDOWN_ONLY);
    ets_delay_us(30);
    if (gpio_get_level(CHARGE_STAT_PIN) != 0) {
        /* STAT=HIGH */
        *state = VCC_CHARG_COMPLETED;
    }
    gpio_set_pull_mode(CHARGE_STAT_PIN, GPIO_PULLUP_ONLY);
    ets_delay_us(30);
    if (gpio_get_level(CHARGE_STAT_PIN) == 0) {
        /* STAT=LOW */
        *state = VCC_CHARG_CHARGING;
    }

    gpio_set_pull_mode(CHARGE_STAT_PIN, GPIO_FLOATING);
    gpio_set_direction(CHARGE_STAT_PIN, GPIO_MODE_DISABLE);
    vTaskDelay(0);
    return ESP_OK;
}
#endif


esp_err_t vcc_vref_enable(void)
{
#if CONFIG_VCC_CHARGE_STAT_PIN == 27
    return ESP_ERR_NOT_SUPPORTED;
#else
    /* Supported gpios are gpios 25, 26, and 27. */
    return adc2_vref_to_gpio(GPIO_NUM_27);
#endif
}
