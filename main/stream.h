#ifndef STREAM_H
#define STREAM_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t stream_init(void);
void stream_begin(void);
void stream_push(const int16_t *s, size_t n);
void stream_end(void);


#endif
