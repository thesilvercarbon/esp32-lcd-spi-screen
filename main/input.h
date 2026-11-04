#ifndef INPUT_H
#define INPUT_H
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

#define BUTTON_PIN  3               // toggle button
#define HOLD_PIN 0    // hold-to-listen button: set to a free GPIO (check the schematic)

typedef void (*input_cb_t)(bool pressed);

esp_err_t input_init(void);                                    // queue, task, ISR service
esp_err_t input_add_button(gpio_num_t pin, input_cb_t cb);     // pull-up, active low

#endif
