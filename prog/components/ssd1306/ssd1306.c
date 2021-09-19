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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

#include "ssd1306.h"
#include "ssd1306_common.h"

#define TAG "ssd1306"

static inline void ssd1306_delay_ms(int ms)
{
    vTaskDelay((ms+portTICK_PERIOD_MS-1)/portTICK_PERIOD_MS);
}

static esp_err_t ssd1306_init_device(ssd1306_t *device, const ssd1306_param_t *param)
{
    gpio_config_t io_conf = {};

    device->width = param->width;
    device->height = param->height;
    device->buffer_size = SSD1306_BUFSIZE(device->width, device->height);
    device->buffer = malloc(device->buffer_size);
    if (device->buffer == NULL) {
        ssd1306_deinit(device);
        return ESP_ERR_NO_MEM;
    }

    device->dc = param->dc;
    device->reset = param->reset;

    gpio_set_level(device->dc, 1);
    gpio_set_level(device->reset, 1);

    io_conf.pin_bit_mask = BIT(device->dc)|BIT(device->reset);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    return ESP_OK;
}

esp_err_t ssd1306_init_gpio(ssd1306_t *device, const ssd1306_param_t *param)
{
    spi_bus_config_t buscfg = {};
    spi_device_interface_config_t devcfg = {};
    esp_err_t err;

    if (param->width < 1 || param->height < 1) {
        ESP_LOGE(TAG, "invalid panel size: %dx%d", param->width, param->height);
        return ESP_ERR_INVALID_ARG;
    }
    memset(device, 0, sizeof(ssd1306_t));

    ESP_LOGD(TAG, "mosi=%d, sclk=%d, cs=%d, dc=%d, reset=%d",
        (int)param->gpio.mosi, (int)param->gpio.sclk,
        (int)param->gpio.cs, (int)param->dc, (int)param->reset);

    buscfg.mosi_io_num = param->gpio.mosi;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = param->gpio.sclk;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER|SPICOMMON_BUSFLAG_SCLK|SPICOMMON_BUSFLAG_MOSI;

    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;
    devcfg.duty_cycle_pos = 0;
    devcfg.clock_speed_hz = 4*1000*1000;
    devcfg.spics_io_num = param->gpio.cs;
    devcfg.queue_size = 7;

    err = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(err);
    err = spi_bus_add_device(VSPI_HOST, &devcfg, &device->spi);
    ESP_ERROR_CHECK(err);
    device->is_spi_managed = true;

    return ssd1306_init_device(device, param);
}

esp_err_t ssd1306_init_spi(ssd1306_t *device, const ssd1306_param_t *param)
{
    if (param->width < 1 || param->height < 1) {
        ESP_LOGE(TAG, "invalid panel size: %dx%d", param->width, param->height);
        return ESP_ERR_INVALID_ARG;
    }
    memset(device, 0, sizeof(ssd1306_t));

    ESP_LOGD(TAG, "dc=%d, reset=%d",
        (int)param->dc, (int)param->reset);

    device->spi = param->spi.spi;
    device->is_spi_managed = false;

    return ssd1306_init_device(device, param);
}

esp_err_t ssd1306_deinit(ssd1306_t *device)
{
    if (device->spi == NULL) {
        return ESP_OK;
    }
    gpio_reset_pin(device->dc);
    gpio_reset_pin(device->reset);
    if (device->is_spi_managed) {
        spi_bus_remove_device(device->spi);
    }
    device->spi = NULL;
    device->is_spi_managed = false;
    if (device->buffer != NULL) {
        free(device->buffer);
        device->buffer = NULL;
        device->buffer_size = 0;
    }
    return ESP_OK;
}

static esp_err_t ssd1306_send(ssd1306_t *device, uint8_t c)
{
    esp_err_t err;
    if (device->spi == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &c;
    err = spi_device_transmit(device->spi, &t);
    assert(err==ESP_OK);
    return err;
}

static esp_err_t ssd1306_send_command(ssd1306_t *device, uint8_t c)
{
    gpio_set_level(device->dc, 0);
    return ssd1306_send(device, c);
}

#define ssd1306_send_commands(device, ...) \
    _ssd1306_send_commands(device, (uint8_t[])__VA_ARGS__, sizeof((uint8_t[])__VA_ARGS__))

static esp_err_t _ssd1306_send_commands(ssd1306_t *device, uint8_t *commands, int cmdlen)
{
    gpio_set_level(device->dc, 0);
    while (cmdlen > 0) {
        esp_err_t err = ssd1306_send(device, *commands++);
        if (err != ESP_OK) {
            return err;
        }
        cmdlen--;
    }
    return ESP_OK;
}

static esp_err_t ssd1306_send_data(ssd1306_t *device, uint8_t d)
{
    gpio_set_level(device->dc, 1);
    return ssd1306_send(device, d);
}

void ssd1306_write(ssd1306_t *device, int x, int y, uint8_t c)
{
    int pos = SSD1306_XY2POS(device, x, y);
    uint8_t b = SSD1306_XY2BIT(device, x, y);
    if (c) {
        device->buffer[pos] |= b;
    } else {
        device->buffer[pos] &= ~b;
    }
    ssd1306_send_commands(device, {
        SSD1306_CMD_SETMAM(2),
        SSD1306_CMD_SETLCSAFORPAM(SSD1306_X2COL(x)),
        SSD1306_CMD_SETHCSAFORPAM(SSD1306_X2COL(x)>>4),
        SSD1306_CMD_SETPAGESTARTADDR(SSD1306_Y2ROW(y)),
    });
    ssd1306_send_data(device, device->buffer[pos]);
}

void ssd1306_send_buffer(ssd1306_t *device, const uint8_t *buffer, int size)
{
    esp_err_t err;
    ssd1306_begin(device);
    gpio_set_level(device->dc, 1);
    spi_transaction_t t = {};
    t.length = size*8;
    t.tx_buffer = buffer;
    err = spi_device_transmit(device->spi, &t);
    ssd1306_end(device);
    ESP_ERROR_CHECK(err);
}

void ssd1306_reset(ssd1306_t *device)
{
    ESP_LOGD(TAG, "> reset");
    memset(device->buffer, 0, device->buffer_size);
    gpio_set_level(device->reset, 0);
    ssd1306_delay_ms(10);
    gpio_set_level(device->reset, 1);
    ssd1306_delay_ms(10);
    ssd1306_send_commands(device, {
        SSD1306_CMD_SETDISPLAYON(0),
        SSD1306_CMD_SETCLOCKDIVOSCFREQ(0, 15),
        SSD1306_CMD_SETMUXRATIO(63),
        SSD1306_CMD_SETDISPLAYOFFSET(0),
        SSD1306_CMD_SETDISPLAYSTARTLINE(0),
        SSD1306_CMD_SETCHARGEPUMP(1),
        SSD1306_CMD_SETSEGMENTREMAP(1),
        SSD1306_CMD_SETCOMOUTSCANDIR(1),
        SSD1306_CMD_SETCOMPINSHWCONF(1, 0),
        SSD1306_CMD_SETCONTRAST(0xFF),
        SSD1306_CMD_SETPRECHAEGEPERIOD(2, 2),
        SSD1306_CMD_SETVCOMHDESELECTLEVEL(4), /* Undocumented value?? */
        SSD1306_CMD_ENTIREDISPLAYON(0),
        SSD1306_CMD_SETINVERTDISPLAY(0),
    });
    ssd1306_flush(device);
    ssd1306_delay_ms(1);
    ssd1306_send_command(device, SSD1306_CMD_SETDISPLAYON(1));
    ESP_LOGD(TAG, "< reset");
}

void ssd1306_on(ssd1306_t *device)
{
    ESP_LOGD(TAG, "> sleep");
    ssd1306_send_command(device, SSD1306_CMD_SETDISPLAYON(1));
    ESP_LOGD(TAG, "< sleep");
}

void ssd1306_sleep(ssd1306_t *device)
{
    ESP_LOGD(TAG, "> sleep");
    ssd1306_send_command(device, SSD1306_CMD_SETDISPLAYON(0));
    ESP_LOGD(TAG, "< sleep");
}

void ssd1306_begin(ssd1306_t *device)
{
    if (++device->lock == 1) {
        spi_device_acquire_bus(device->spi,  portMAX_DELAY);
    }
}

void ssd1306_end(ssd1306_t *device)
{
    if (--device->lock == 0) {
        spi_device_release_bus(device->spi);
    }
}

void ssd1306_flush(ssd1306_t *device)
{
    ssd1306_send_commands(device, {
        SSD1306_CMD_SETMAM(1),
        SSD1306_CMD_SETCOLADDR(0, SSD1306_X2COL(device->width-1)),
        SSD1306_CMD_SETPAGEADDR(0, SSD1306_Y2ROW(device->height-1)),
    });
    ssd1306_send_buffer(device, device->buffer, device->buffer_size);
}
