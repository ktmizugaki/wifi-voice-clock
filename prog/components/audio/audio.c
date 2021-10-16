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
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_event.h>

#include "audio.h"
#include "audio_event.h"

#define TAG "audio"

#define AUDIO_I2S_NUM           (I2S_NUM_0)
#define AUDIO_I2S_SAMPLE_RATE   (16000)
#define AUDIO_I2S_BUFF_LEN      (4 * 1024)

#define AUDIO_DMA_BUF_COUNT     2
#define AUDIO_DMA_BUF_LEN       1024

#define AUDIO_USE_INTERNAL_DAC  0
#define AUDIO_USE_AMP           0

#if !AUDIO_USE_INTERNAL_DAC
#define AUDIO_OUT_PIN       GPIO_NUM_25
#endif

#if AUDIO_USE_AMP
#define AUDIO_AMP_EN_PIN    GPIO_NUM_26
#endif

#define DMA_SIZE    (AUDIO_DMA_BUF_COUNT * AUDIO_DMA_BUF_LEN)

ESP_EVENT_DEFINE_BASE(AUDIO_EVENT);

typedef struct {
    bool playing;
    bool stop;
    QueueHandle_t queue;
    TaskHandle_t task_handle;
    uint8_t *read_buff;
    uint8_t *i2s_write_buff;
    uint32_t samplerate;
} audio_task_t;

typedef struct {
    audio_data_func_t func;
    void *arg;
    uint32_t samplerate;
    int channels;
    int bps;
} audio_item_t;

static int audio_convert_8bit(int16_t* d_buff, uint8_t* s_buff, int size)
{
    int i, j;
    for (i = 0, j = 0; i < size; i++, j++) {
#if AUDIO_USE_INTERNAL_DAC
        /* DAC uses only highest 8bit data value. */
        d_buff[j] = (s_buff[i]<<8);
#else
        d_buff[j++] = s_buff[i]*257 - 0x8000;
#endif
    }
    return j*2;
}

static int audio_convert_16bit(int16_t* d_buff, int16_t* s_buff, int size)
{
    int i, j;
    size /= 2;
    for (i = 0, j = 0; i < size; i++, j++) {
#if AUDIO_USE_INTERNAL_DAC
        /* DAC uses only highest 8bit data value. */
        d_buff[j] = s_buff[i] + 0x8000;
#else
        d_buff[j] = s_buff[i];
#endif
    }
    return j*2;
}

static void play_task(audio_task_t *task)
{
    uint8_t *read_buff = task->read_buff;
    uint8_t *i2s_write_buff = task->i2s_write_buff;
    int length;
    audio_item_t item;
    bool item_done = false;

    if (xQueuePeek(task->queue, &item, 0) != pdTRUE) {
        ESP_LOGV(TAG, "no item");
        task->stop = true;
        return;
    }
    length = AUDIO_I2S_BUFF_LEN;
    ESP_LOGV(TAG, "read item(%p)", item.arg);
    if (item.func(item.arg, read_buff, &length) == 0) {
        /* remove item from queue */
        xQueueReceive(task->queue, &item, 0);
        item_done = true;
    }
    if (length > 0) {
        ESP_LOGV(TAG, "item(%p): %d", item.arg, length);
        int offset = 0;
        if (task->samplerate != item.samplerate) {
            ESP_LOGV(TAG, "play task: change samplerate: %d -> %d", task->samplerate, item.samplerate);
            task->samplerate = item.samplerate;
            i2s_set_sample_rates(AUDIO_I2S_NUM, task->samplerate);
        }
        ESP_LOGV(TAG, "play start");
        while (!task->stop && offset < length) {
            int size = (length - offset) < DMA_SIZE ? (length - offset) : DMA_SIZE;
            size_t i2s_write_len;
            if (item.bps == 8) {
                i2s_write_len = audio_convert_8bit((int16_t*)i2s_write_buff, (read_buff + offset), size);
            } else {
                i2s_write_len = audio_convert_16bit((int16_t*)i2s_write_buff, (int16_t*)(read_buff + offset), size);
            }
            i2s_write(AUDIO_I2S_NUM, i2s_write_buff, i2s_write_len, &i2s_write_len, portMAX_DELAY);
            offset += size;
        }
        ESP_LOGV(TAG, "play done");
    }
    if (item_done) {
        item.func(item.arg, NULL, NULL);
        ESP_LOGV(TAG, "item(%p) done", item.arg);
    }
}

static void audio_task(void *arg)
{
    audio_task_t *task = (audio_task_t*)arg;
    audio_item_t item;

    task->read_buff = malloc(AUDIO_I2S_BUFF_LEN);
    task->i2s_write_buff = malloc(DMA_SIZE);
    task->samplerate = AUDIO_I2S_SAMPLE_RATE;

    while (1) {
#if AUDIO_USE_AMP
        gpio_set_level(AUDIO_AMP_EN_PIN, 1);
        gpio_set_direction(AUDIO_AMP_EN_PIN, GPIO_MODE_OUTPUT);
#endif
        i2s_stop(AUDIO_I2S_NUM);
        task->playing = false;
#if !AUDIO_USE_INTERNAL_DAC
        gpio_set_level(AUDIO_OUT_PIN, 0);
        gpio_set_direction(AUDIO_OUT_PIN, GPIO_MODE_DISABLE);
#endif
        while (task->stop) {
            ESP_LOGV(TAG, "play task: stopping");
            vTaskSuspend(NULL);
        }
        task->playing = true;
        esp_event_post(AUDIO_EVENT, AUDIO_EVENT_STARTED, NULL, 0, 0);
#if !AUDIO_USE_INTERNAL_DAC
        gpio_set_direction(AUDIO_OUT_PIN, GPIO_MODE_OUTPUT);
        i2s_pin_config_t pin_config = {
            .bck_io_num = I2S_PIN_NO_CHANGE,
            .ws_io_num = I2S_PIN_NO_CHANGE,
            .data_out_num = AUDIO_OUT_PIN,
            .data_in_num = I2S_PIN_NO_CHANGE,
        };
        i2s_set_pin(AUDIO_I2S_NUM, &pin_config);
#endif
        i2s_zero_dma_buffer(AUDIO_I2S_NUM);
        i2s_start(AUDIO_I2S_NUM);
#if AUDIO_USE_AMP
        gpio_set_level(AUDIO_AMP_EN_PIN, 0);
#endif
        while (!task->stop) {
            play_task(task);
        }
        /* release resources */
        while (xQueueReceive(task->queue, &item, 0) == pdTRUE) {
            ESP_LOGV(TAG, "fill task: discard item(%p)", item.arg);
            item.func(item.arg, NULL, NULL);
        }
        esp_event_post(AUDIO_EVENT, AUDIO_EVENT_STOPPED, NULL, 0, 0);
    }
    free(task->read_buff);
    free(task->i2s_write_buff);
    vTaskDelete(NULL);
}

static audio_task_t s_audio_task;
static bool s_audio_initialized = false;

esp_err_t audio_init(void)
{
    esp_err_t err;
    if (s_audio_initialized) {
        return ESP_OK;
    }
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX
#if AUDIO_USE_INTERNAL_DAC
                    | I2S_MODE_DAC_BUILT_IN
#else
                    | I2S_MODE_PDM
#endif
        ,
        .sample_rate =  AUDIO_I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .communication_format =
#if AUDIO_USE_INTERNAL_DAC
            I2S_COMM_FORMAT_I2S_MSB
#else
            I2S_COMM_FORMAT_PCM
#endif
        ,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .intr_alloc_flags = 0,
        .dma_buf_count = AUDIO_DMA_BUF_COUNT,
        .dma_buf_len = AUDIO_DMA_BUF_LEN,
    };
    /* install and start i2s driver */
    err = i2s_driver_install(AUDIO_I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        return err;
    }
#if AUDIO_USE_INTERNAL_DAC
    /* init DAC pad */
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
#else
    gpio_set_level(AUDIO_OUT_PIN, 0);
    gpio_set_direction(AUDIO_OUT_PIN, GPIO_MODE_DISABLE);
#endif
#if AUDIO_USE_AMP
    gpio_set_level(AUDIO_AMP_EN_PIN, 1);
    gpio_set_direction(AUDIO_AMP_EN_PIN, GPIO_MODE_OUTPUT);
#endif

    /* 何をしたいのか分からなくなってきた。 */
    s_audio_task.playing = false;
    s_audio_task.stop = true;
    s_audio_task.queue = xQueueCreate(10, sizeof(audio_item_t));
    if (xTaskCreate(audio_task, "audio_task", 8 * 1024, &s_audio_task, 1, &s_audio_task.task_handle) != pdTRUE) {
        vQueueDelete(s_audio_task.queue);
        return ESP_ERR_NO_MEM;
    }
    s_audio_initialized = true;
    return ESP_OK;
}

void audio_stop(void)
{
    if (!s_audio_initialized) {
        return;
    }
    if (s_audio_task.playing) {
        s_audio_task.stop = true;
        audio_wait();
    }
}

void audio_wait(void)
{
    if (!s_audio_initialized) {
        return;
    }
    while (s_audio_task.playing) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

bool audio_is_playing(void)
{
    return s_audio_initialized && s_audio_task.playing;
}

#define BEEP_V  0x7fff
#define BEEP_S  15

struct beep {
    int length;
    int cycle;
    int count;
    int x, y, c, s;
};

static int audio_beep_data_func(void *arg, void *data, int *size)
{
    struct beep *beep = (struct beep*)arg;
    int16_t *pdata = (int16_t *)data;
    int x0, y0;
    int i;
    if (data == NULL && size == NULL) {
        free(beep);
        return 0;
    }
    for (i = 0; i < *size; i+=2) {
        *pdata++ = beep->y>>3;
        x0 = beep->x, y0 = beep->y;
        beep->x = (x0*beep->c - y0*beep->s)>>BEEP_S;
        beep->y = (x0*beep->s + y0*beep->c)>>BEEP_S;
        if (--beep->count <= 0) {
            if (--beep->cycle < 0) {
                break;
            }
            beep->count += beep->length;
            beep->x = BEEP_V-1;
            beep->y = 0;
        }
    }
    *size = i;
    return beep->count > 0;
}

void audio_beep(int frequency, int duration)
{
    struct beep *beep;
    if (!s_audio_initialized) {
        return;
    }
    beep = malloc(sizeof(struct beep));
    if (frequency == 0) {
        beep->length = AUDIO_I2S_SAMPLE_RATE;
        beep->cycle = duration/1000;
        beep->count = duration%1000;
        beep->x = 0;
        beep->y = 0;
        beep->c = 0;
        beep->s = 0;
    } else {
        float angle = (float)(frequency*2*M_PI)/AUDIO_I2S_SAMPLE_RATE, c, s;
        sincosf(angle, &s, &c);
        beep->length = AUDIO_I2S_SAMPLE_RATE*4/frequency;
        beep->cycle = (duration*AUDIO_I2S_SAMPLE_RATE/1000+beep->length)/beep->length;
        beep->count = 0;
        beep->x = BEEP_V;
        beep->y = 0;
        beep->c = (int)(BEEP_V*c+0.5f);
        beep->s = (int)(BEEP_V*s+0.5f);
    }
    ESP_LOGV(TAG, "add beep item(%p)", beep);
    audio_play(audio_beep_data_func, (void*)beep,
        AUDIO_I2S_SAMPLE_RATE, 1, 16, AUDIO_IMMEDIATE);
}

void audio_play(audio_data_func_t func, void *arg,
    int samplerate, int channels, int bits,
    audio_queue_mode_t mode)
{
    if (!s_audio_initialized) {
        func(arg, NULL, NULL);
        return;
    }
    audio_item_t item = {
        .func = func,
        .arg = arg,
        .samplerate = samplerate,
        .channels = channels,
        .bps = bits,
    };
    if (mode == AUDIO_REPLACE) {
        audio_stop();
    }
    ESP_LOGV(TAG, "add play item(%p)", item.arg);
    if (mode == AUDIO_IMMEDIATE) {
        xQueueSendToFront(s_audio_task.queue, &item, portMAX_DELAY);
    } else {
        xQueueSendToBack(s_audio_task.queue, &item, portMAX_DELAY);
    }
    s_audio_task.stop = false;
    vTaskResume(s_audio_task.task_handle);
}
