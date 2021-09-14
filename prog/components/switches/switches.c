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
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_log.h>

#include "switches.h"

#define TAG "switches"

#define DEBOUNCE_TICK   (10/portTICK_PERIOD_MS)
#define HOLD_TICK       (2*1000/portTICK_PERIOD_MS)

#define SWITCH_FLAG_DEBOUNCING  (1<<0)
#define SWITCH_FLAG_WAITING_HOLD    (1<<1)

#define SWITCH_DEBOUNCING(sw)   ((sw)->flags & SWITCH_FLAG_DEBOUNCING)
#define SWITCH_WAITING_HOLD(sw)   ((sw)->flags & SWITCH_FLAG_WAITING_HOLD)

/* passed from ISR to task */
typedef struct {
    uint8_t id;
    uint8_t pin;
    TickType_t time;
} switch_event_t;

/* information and state of individual switch */
typedef struct {
    uint8_t id;
    uint8_t pin;
    uint8_t level;
    uint8_t flags;
    TickType_t time;
} switch_state_t;

/* holds entire state of switches */
typedef struct {
    TaskHandle_t task_handle;
    QueueHandle_t queue;
    switches_callback_t *callback;
    int switch_count;
    switch_state_t *states;
    /* combination of currently ON switches + whether held long */
    enum switch_flags state;
    uint8_t flags;
    /* debouncing time of most near future */
    TickType_t debounce_time;
    /* hold time */
    TickType_t hold_time;
} switches_t;

static switches_t s_switches = {
    .task_handle = NULL,
    .queue = NULL,
    .callback = NULL,
    .switch_count = 0,
    .states = NULL,
    .state = 0,
    .flags = 0,
    .debounce_time = 0,
    .hold_time = 0,
};

static esp_err_t switches_init2(switches_t *switches,
    switches_callback_t *callback, int pin_count, const uint8_t *pins);
static void switches_deinit2(switches_t *switches);
static esp_err_t switches_start(switches_t *switches);
static void switches_stop(switches_t *switches);

static void switches_gpio_isr_handler(void *arg)
{
    const switch_state_t *sw = arg;
    // FIXME: switches should not be hard coded.
    if (s_switches.queue != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        switch_event_t event;
        event.id = sw->id;
        event.pin = sw->pin;
        event.time = xTaskGetTickCountFromISR();
        xQueueSendFromISR(s_switches.queue, &event, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

static TickType_t switches_calc_timeout(const switches_t *switches)
{
    TickType_t time = xTaskGetTickCount();
    TickType_t timeout = portMAX_DELAY;
    int d;
    if (SWITCH_DEBOUNCING(switches)) {
        d = switches->debounce_time - time;
        timeout = MIN(timeout, MAX(0, d));
    }
    if (SWITCH_WAITING_HOLD(switches)) {
        d = switches->hold_time - time;
        timeout = MIN(timeout, MAX(0, d));
    }
    return timeout;
}

static void switch_process_event(const switch_event_t *event, switch_state_t *sw, switches_t *switches)
{
    if (!SWITCH_DEBOUNCING(sw)) {
        ESP_LOGV(TAG, "sw%d: start debounce", sw->id);
        sw->flags |= SWITCH_FLAG_DEBOUNCING;
        sw->time = event->time+DEBOUNCE_TICK;
        if (!SWITCH_DEBOUNCING(switches)) {
            switches->flags |= SWITCH_FLAG_DEBOUNCING;
            switches->debounce_time = sw->time;
        }
    } else {
        ESP_LOGV(TAG, "sw%d: debouncing", sw->id);
    }
}

static void switch_changed(switch_state_t *sw, switches_t *switches)
{
    enum switch_flags prev_state, action;
    const unsigned int key = 1<<sw->id;

    prev_state = switches->state;
    action = key|switches->state;
    ESP_LOGD(TAG, "sw%d: %s", sw->id, sw->level? "pressed": "released");
    switches->state &= ~SWITCH_HOLD;
    if (sw->level) {
        switches->flags |= SWITCH_FLAG_WAITING_HOLD;
        switches->hold_time = sw->time+HOLD_TICK;
        switches->state |= key;
        action |= SWITCH_PRESS;
    } else {
        switches->flags &= ~SWITCH_FLAG_WAITING_HOLD;
        switches->state &= ~key;
        action |= SWITCH_RELEASE;
    }
    if (switches->callback != NULL) {
        switches->callback(action, prev_state);
    }
}

static void switch_check_debounce(switch_state_t *sw, TickType_t time, switches_t *switches)
{
    int d;
    int level;
    if (SWITCH_DEBOUNCING(sw)) {
        d = (sw->time - time);
        if (d <= 0) {
            sw->flags &= ~SWITCH_FLAG_DEBOUNCING;
            level = gpio_get_level(sw->pin);
            ESP_LOGV(TAG, "sw%d: debounce ended, level: %d->%d", sw->id, sw->level, level);
            if (sw->level != level) {
                sw->level = level;
                switch_changed(sw, switches);
            }
        }
    }
}

static void switches_check_debounce(switches_t *switches)
{
    int i;
    int d, delay = INT_MAX;
    TickType_t time = xTaskGetTickCount();
    d = switches->debounce_time - time;
    if (SWITCH_DEBOUNCING(switches) && d <= 0) {
        ESP_LOGV(TAG, "checking debounce: %u,%u", switches->debounce_time, time);
        for (i = 0; i < switches->switch_count; i++) {
            switch_state_t *sw = &switches->states[i];
            d = sw->time - switches->debounce_time;
            if (SWITCH_DEBOUNCING(sw) && d <= 0) {
                switch_check_debounce(sw, time, switches);
            }
            if (SWITCH_DEBOUNCING(sw)) {
                d = sw->time-time;
                delay = MIN(delay, d);
            }
        }
        if (delay >= INT_MAX) {
            switches->flags &= ~SWITCH_FLAG_DEBOUNCING;
        }
        switches->debounce_time = time + delay;
        if (SWITCH_DEBOUNCING(switches)) {
            ESP_LOGV(TAG, "next debounce time: %u", switches->debounce_time);
        }
    }
}

static void switches_check_hold(switches_t *switches)
{
    enum switch_flags prev_state;
    switches_callback_t *callback;
    int d;
    TickType_t time = xTaskGetTickCount();
    d = switches->hold_time - time;
    if (SWITCH_WAITING_HOLD(switches)) {
        ESP_LOGV(TAG, "hold time: %d", d);
    }
    if (SWITCH_WAITING_HOLD(switches) && d <= 0) {
        switches->flags &= ~SWITCH_FLAG_WAITING_HOLD;
        prev_state = switches->state;
        switches->state |= SWITCH_HOLD;
        callback = switches->callback;
        if (callback != NULL) {
            callback(switches->state, prev_state);
        }
    }
}

static void switches_task(void *arg)
{
    switches_t *switches = arg;
    switch_event_t event;
    int loop = 5;
    while (switches->task_handle != NULL && switches->queue != NULL) {
        TickType_t timeout = switches_calc_timeout(switches);
        if (timeout == 0) {
            --loop;
        } else {
            loop = 5;
        }
        if (loop > 0 && xQueueReceive(switches->queue, &event, timeout)) {
            ESP_LOGV(TAG, "event: %d(%d)@%d", event.id, event.pin, event.time);
            if (event.id < switches->switch_count) {
                switch_process_event(&event, &switches->states[event.id], switches);
            }
        } else {
            switches_check_debounce(switches);
            switches_check_hold(switches);
            loop = 5;
        }
    }
}

/*
 * switches
 */

static esp_err_t switches_init_gpio(switches_t *switches)
{
    int i;
    TickType_t time = xTaskGetTickCount();
    gpio_config_t io_conf = {};
    esp_err_t err;

    for (i = 0; i < switches->switch_count; i++) {
        switch_state_t *sw = &switches->states[i];
        io_conf.pin_bit_mask |= BIT64(sw->pin);
    }

    io_conf.mode = GPIO_MODE_INPUT;
    /* pins are pulled down externally and switches are connected to Vcc */
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0; i < switches->switch_count; i++) {
        switch_state_t *sw = &switches->states[i];
        sw->time = time;
        sw->level = gpio_get_level(sw->pin);
        sw->flags = 0;
        if (sw->level) {
            switches->flags |= SWITCH_FLAG_WAITING_HOLD;
            switches->hold_time = sw->time+HOLD_TICK;
            switches->state |= (1<<sw->id);
        }
    }

    return ESP_OK;
}

static esp_err_t switches_init2(switches_t *switches,
    switches_callback_t *callback, int pin_count, const uint8_t *pins)
{
    int i;
    esp_err_t err;

    switches->callback = callback;
    switches->switch_count = pin_count;
    switches->queue = xQueueCreate(8, sizeof(switch_event_t));
    switches->states = malloc(sizeof(switch_state_t) * pin_count);
    if (switches->queue == NULL || switches->states == NULL) {
        switches_deinit2(switches);
        return ESP_FAIL;
    }

    for (i = 0; i < pin_count; i++) {
        switch_state_t *sw = &switches->states[i];
        sw->id = i;
        sw->pin = pins[i];
    }

    err = switches_init_gpio(switches);
    if (err != ESP_OK) {
        switches_deinit2(switches);
        return err;
    }

    return ESP_OK;
}

static void switches_deinit2(switches_t *switches)
{
    switches->callback = NULL;
    switches->switch_count = 0;
    if (switches->states != NULL) {
        free(switches->states);
        switches->states = NULL;
    }
    if (switches->queue != NULL) {
        vQueueDelete(switches->queue);
        switches->queue = NULL;
    }
    switches->state = 0;
    switches->flags = 0;
}

static esp_err_t switches_start_gpio_isr(switches_t *switches)
{
    int i;
    esp_err_t err;

    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        return err;
    }

    for (i = 0; i < switches->switch_count; i++) {
        switch_state_t *sw = &switches->states[i];
        err = gpio_isr_handler_add(sw->pin, switches_gpio_isr_handler, sw);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

static esp_err_t switches_start(switches_t *switches)
{
    esp_err_t err;
    if (switches->task_handle != NULL) {
        ESP_LOGD(TAG, "switches is already started");
        return ESP_ERR_INVALID_STATE;
    }

    if (xTaskCreate(switches_task, "switches_task", 34*1024, switches, 5, &switches->task_handle) != pdTRUE) {
        switches->task_handle = NULL;
        switches_stop(switches);
        return ESP_FAIL;
    }

    err = switches_start_gpio_isr(switches);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

static void switches_stop(switches_t *switches)
{
    gpio_uninstall_isr_service();
    if (switches->task_handle != NULL) {
        TaskHandle_t task_handle = switches->task_handle;
        switches->task_handle = NULL;

        if (switches->queue != NULL) {
            /* wakeup task by sending invalid event */
            switch_event_t event = {0xff, 0xff, portMAX_DELAY};
            xQueueSend(switches->queue, &event, 0);
            vTaskDelay(1);
        }

        vTaskDelete(task_handle);
        if (switches->queue != NULL) {
            xQueueReset(switches->queue);
        }
    }
}

/*
 * api
 */

esp_err_t switches_init(switches_callback_t *callback, int pin_count, const uint8_t *pins)
{
    esp_err_t err;

    if (callback == NULL) {
        ESP_LOGE(TAG, "callback is required");
        return ESP_ERR_INVALID_ARG;
    }
    if (pin_count <= 0) {
        ESP_LOGE(TAG, "at least one switch is required");
        return ESP_ERR_INVALID_ARG;
    }
    if (pin_count > SWITCH_MAX_KEY) {
        ESP_LOGE(TAG, "too many switches");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_switches.queue != NULL) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    err = switches_init2(&s_switches, callback, pin_count, pins);
    if (err != ESP_OK) {
        switches_deinit();
        return err;
    }

    err = switches_start(&s_switches);
    if (err != ESP_OK) {
        switches_deinit();
        return err;
    }

    return ESP_OK;
}

void switches_deinit(void)
{
    switches_stop(&s_switches);
    switches_deinit2(&s_switches);
}

enum switch_flags switches_get_state(void)
{
    return s_switches.state;
}
