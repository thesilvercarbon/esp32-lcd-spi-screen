#ifndef MIC_H
#define MIC_H
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define MIC_SCK_PIN 1
#define MIC_WS_PIN 2
#define MIC_SD_PIN 23
#define MIC_SAMPLE_RATE 16000

esp_err_t mic_init(void);
esp_err_t mic_read(int32_t *buf, size_t n, size_t *got);

esp_err_t mic_start(void);
esp_err_t mic_stop(void);

#endif


