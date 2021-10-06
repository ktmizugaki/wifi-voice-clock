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

#ifdef __cplusplus
extern "C" {
#endif

/** deep sleep with timer and switch wakeup. */
extern void power_suspend(void) __attribute__((noreturn));
/** deep sleep with switch wakeup, without timer. */
extern void power_hibernate(void) __attribute__((noreturn));
/** deep sleep, no wakeup. */
extern void power_halt(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
