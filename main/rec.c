#include "rec.h"
#include "mic.h"
#include "stream.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

#define CHUNK 240

static const char *TAG = "rec";
static volatile bool s_active;
static TaskHandle_t s_task;

static void rec_task(void *args){
    int32_t in[CHUNK];
    int16_t out[CHUNK];
    for (;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        size_t n = 0;
        int peak = 0;
        if (mic_start() != ESP_OK){
            ESP_LOGE(TAG, "mic_start failed");
            continue;
        }
        stream_begin();
        while (s_active){
            size_t got;
            if (mic_read(in, CHUNK, &got) != ESP_OK) break;
            size_t frames = got / 2;                    // 2 words per frame: [mic, empty]
            for (size_t i = 0; i < frames; i++){
                out[i] = (int16_t)(in[2 * i] >> 16);
                if (abs(out[i]) > peak) peak = abs(out[i]);
            }
            stream_push(out, frames);
            n += frames;
        }
        stream_end();
        mic_stop();
        ESP_LOGI(TAG, "recorded %u samples (%u ms) peak=%d free heap=%lu",
                 (unsigned)n, (unsigned)(n * 1000 / MIC_SAMPLE_RATE), peak,
                 (unsigned long)esp_get_free_heap_size());
    }
}

esp_err_t rec_init(void){
    return xTaskCreate(rec_task, "rec", 4096, NULL, 4, &s_task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

void rec_begin(void) { s_active = true; xTaskNotifyGive(s_task); }
void rec_end(void) { s_active = false; }
