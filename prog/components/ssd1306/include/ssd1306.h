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

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    short width;
    short height;
    union {
        struct {
            gpio_num_t mosi;    /* GPIO_NUM_23 */
            gpio_num_t sclk;    /* GPIO_NUM_18 */
            gpio_num_t cs;
        } gpio;
        struct {
            spi_device_handle_t spi;
        } spi;
    };
    gpio_num_t dc;
    gpio_num_t reset;
} ssd1306_param_t;

typedef struct {
    short width;
    short height;
    spi_device_handle_t spi;
    bool is_spi_managed;
    gpio_num_t dc;
    gpio_num_t reset;
    uint8_t *buffer;
    size_t buffer_size;
    int lock;
} ssd1306_t;

extern esp_err_t ssd1306_init_gpio(ssd1306_t *device, const ssd1306_param_t *param);
extern esp_err_t ssd1306_init_spi(ssd1306_t *device, const ssd1306_param_t *param);
extern esp_err_t ssd1306_deinit(ssd1306_t *device);

extern void ssd1306_reset(ssd1306_t *device);
extern void ssd1306_on(ssd1306_t *device);
extern void ssd1306_sleep(ssd1306_t *device);
extern void ssd1306_begin(ssd1306_t *device);
extern void ssd1306_end(ssd1306_t *device);
extern void ssd1306_flush(ssd1306_t *device);

extern void ssd1306_write(ssd1306_t *device, int x, int y, uint8_t c);
extern void ssd1306_send_buffer(ssd1306_t *device,
    const uint8_t *data, int size);

#ifdef __cplusplus
}
#endif
