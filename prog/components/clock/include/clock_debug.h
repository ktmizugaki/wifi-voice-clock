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

#include <esp_err.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * debug ESP32 RTC.
 * miscellaneous debugging functions when I test esp32's rtc with 32k xtal.
 */

/** @brief print value of rtc time. */
extern void clock_debug_print_time(void);
/**
 * @brief check if 32k xtal is oscillating correctly.
 * output clock of 32k xtal to clkout_pin, count clock with pcnt_pin.
 * then it prints message each time pcnt counted specific number of clock,
 * so you can see if 32k xtal is oscillating or not.
 * @note connect clkout_pin and pcnt_pin externally when using this function.
 * @warning only use this function in debuging build. output cannot be stopped.
 * @param[in] clkout_pin pin to output clock. must be either GPIO 25 or GPIO 26.
 * @param[in] pcnt_pin   pin to read clock with PCNT module.
 * @return ESP_OK for success, other value fo failure.
 * @see https://esp32.com/viewtopic.php?t=1570
 */
extern esp_err_t clock_debug_32k_xtal(gpio_num_t clkout_pin, gpio_num_t pcnt_pin);
/**
 * @brief stop task and pcnt started by @ref clock_debug_32k_xtal.
 * @warning this does not stop clock ouput because I don't know how.
 */
extern void clock_debug_stop(void);

#ifdef __cplusplus
}
#endif
