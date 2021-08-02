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

#include <stdint.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rtc.h>
#include <esp32/clk.h>
#include <esp_err.h>
#include <esp_log.h>

#include <driver/gpio.h>
#include <driver/pcnt.h>

#include "clock.h"
#include "clock_debug.h"

#define TAG "clockdbg"

static pcnt_isr_handle_t s_pcnt_isr_handle = NULL; //user's ISR service handle
static TaskHandle_t s_task_handle = NULL;
static int s_pcnt_val, s_pcnt_time;
static time_t s_clock_time;

static void IRAM_ATTR pcnt_xtal_intr_handler(void *arg)
{
    uint32_t intr_status = PCNT.int_st.val;
    int i = PCNT_UNIT_7;

    if (intr_status & (BIT(i))) {
        int val = PCNT.status_unit[i].val;
        int time = xTaskGetTickCountFromISR();
        PCNT.int_clr.val = BIT(i);
        s_pcnt_val = val;
        s_pcnt_time = time;
        vTaskNotifyGiveFromISR(s_task_handle, NULL);
    }
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == CLOCK && event_id == CLOCK_EVENT_SECOND) {
        s_clock_time = *(time_t*)event_data;
        xTaskNotifyGive(s_task_handle);
    }
}

static esp_err_t xtal_test_init(gpio_num_t clkout_pin, gpio_num_t pcnt_pin)
{
    /* https://esp32.com/viewtopic.php?t=1570 */
    #include "soc/rtc_io_reg.h"
    #include "soc/sens_reg.h"

    if (s_pcnt_isr_handle != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    rtc_clk_32k_enable(true);
    if (clkout_pin == GPIO_NUM_25) {
        REG_SET_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_MUX_SEL_M);
        REG_CLR_BIT(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_RDE_M | RTC_IO_PDAC1_RUE_M);
        REG_SET_FIELD(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_FUN_SEL, 1);
    } else if (clkout_pin == GPIO_NUM_26) {
        REG_SET_BIT(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_MUX_SEL_M);
        REG_CLR_BIT(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_RDE_M | RTC_IO_PDAC2_RUE_M);
        REG_SET_FIELD(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_FUN_SEL, 1);
    } else {
        ESP_LOGE(TAG, "invalid clkout_pin: %d", clkout_pin);
        return ESP_FAIL;
    }
    REG_SET_FIELD(SENS_SAR_DAC_CTRL1_REG, SENS_DEBUG_BIT_SEL, 0);
    /* sel = 4 : 32k XTAL; sel = 5 : internal 150k RC */
    REG_SET_FIELD(RTC_IO_RTC_DEBUG_SEL_REG, RTC_IO_DEBUG_SEL0, 4);

    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pcnt_pin,
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
        .counter_h_lim = 32767,
        .counter_l_lim = 0,
        .unit = PCNT_UNIT_7,
        .channel = PCNT_CHANNEL_0,
    };
    /* Initialize PCNT unit */
    ESP_ERROR_CHECK( pcnt_unit_config(&pcnt_config) );

    pcnt_counter_pause(pcnt_config.unit);
    pcnt_counter_clear(pcnt_config.unit);

    pcnt_event_enable(pcnt_config.unit, PCNT_EVT_ZERO);
    //pcnt_event_enable(pcnt_config.unit, PCNT_EVT_H_LIM);
    //pcnt_event_enable(pcnt_config.unit, PCNT_EVT_L_LIM);

    ESP_ERROR_CHECK( pcnt_isr_register(pcnt_xtal_intr_handler, NULL, 0, &s_pcnt_isr_handle) );
    pcnt_intr_enable(pcnt_config.unit);

    pcnt_counter_resume(pcnt_config.unit);
    return ESP_OK;
}

static void xtal_test_deinit(void)
{
    pcnt_unit_t unit = PCNT_UNIT_7;
    pcnt_intr_disable(unit);
    pcnt_event_disable(unit, PCNT_EVT_ZERO);
    pcnt_event_disable(unit, PCNT_EVT_H_LIM);
    pcnt_event_disable(unit, PCNT_EVT_L_LIM);

    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    esp_intr_free(s_pcnt_isr_handle);
    s_pcnt_isr_handle = NULL;
}

static void xtal_test_task(void *arg)
{
    int pcnt_val, pcnt_time;
    time_t clock_time;
    int16_t pcnt_curr;
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        pcnt_val = s_pcnt_val;
        s_pcnt_val = 0;
        pcnt_time = s_pcnt_time;
        s_pcnt_time = 0;
        clock_time = s_clock_time;
        s_clock_time = 0;
        if (pcnt_val != 0) {
            ESP_LOGI(TAG, "pcnt overflow: %d: %d", pcnt_time, pcnt_val);
        }
        if (clock_time != 0) {
            pcnt_get_counter_value(PCNT_UNIT_7, &pcnt_curr);
            ESP_LOGI(TAG, "clock : %lu: %d: %d",
                (unsigned long)clock_time, (int)xTaskGetTickCount(), pcnt_curr);
        }
        ESP_LOGI(TAG, "HighWaterMark: %u", (unsigned)uxTaskGetStackHighWaterMark(NULL));
    }
}

void clock_debug_print_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t rt = esp_clk_rtc_time();
    ESP_LOGD(TAG, "%ld.%06lu, %lu.%06lu, %llu, %u",
        tv.tv_sec, tv.tv_usec, (unsigned long)(rt/1000000), (unsigned long)(rt%1000000),
        rtc_time_get(), esp_clk_slowclk_cal_get());
}

/* clkout_pin is either 25 or 26 */
esp_err_t clock_debug_32k_xtal(gpio_num_t clkout_pin, gpio_num_t pcnt_pin)
{
    BaseType_t ret;
    esp_err_t err;

    if (s_task_handle != NULL) {
        return ESP_OK;
    }
    ret = xTaskCreate(xtal_test_task, "clock_task", 8*1024, NULL, 20,
        &s_task_handle);
    if (ret != pdPASS) {
        s_task_handle = NULL;
        return ESP_FAIL;
    }
    err = xtal_test_init(clkout_pin, pcnt_pin);
    if (err != ESP_OK) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
        return ESP_FAIL;
    }
    clock_register_event_handler(event_handler, NULL);

    return ESP_OK;
}

void clock_debug_stop(void)
{
    if (s_task_handle != NULL) {
        xtal_test_deinit();

        vTaskDelay(10);

        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }
}
