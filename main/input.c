#include "input.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define MAX_BUTTONS    4
#define SETTLE_MS      20
#define PRESSED_LEVEL  0                       // pull-up wiring

typedef struct { gpio_num_t pin; input_cb_t cb; int last; } btn_t;

static const char *TAG = "input";
static btn_t         s_btn[MAX_BUTTONS];
static int           s_count;
static QueueHandle_t s_q;

static void IRAM_ATTR button_isr(void *arg)
{
    uint8_t idx = (uint8_t)(uintptr_t)arg;     // which button, passed at handler_add
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(s_q, &idx, &woken);
    portYIELD_FROM_ISR(woken);
}

static void input_task(void *arg)
{
    uint8_t idx;
    for (;;) {
        if (!xQueueReceive(s_q, &idx, portMAX_DELAY)) continue;
        vTaskDelay(pdMS_TO_TICKS(SETTLE_MS));          // let the contact settle

        btn_t *b = &s_btn[idx];
        int level = gpio_get_level(b->pin);
        if (level == b->last) continue;                // bounce leftovers, nothing changed
        b->last = level;
        ESP_LOGI(TAG, "pin %d %s", b->pin, level == PRESSED_LEVEL ? "down" : "up");
        if (b->cb) b->cb(level == PRESSED_LEVEL);
    }
}

esp_err_t input_init(void)
{
    s_q = xQueueCreate(8, sizeof(uint8_t));
    if (!s_q) return ESP_ERR_NO_MEM;
    if (xTaskCreate(input_task, "input", 4096, NULL, 3, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    esp_err_t err = gpio_install_isr_service(0);
    return (err == ESP_ERR_INVALID_STATE) ? ESP_OK : err;   // already installed is fine
}

esp_err_t input_add_button(gpio_num_t pin, input_cb_t cb)
{
    if (s_count >= MAX_BUTTONS) return ESP_ERR_NO_MEM;
    int idx = s_count;
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    s_btn[idx] = (btn_t){ .pin = pin, .cb = cb, .last = gpio_get_level(pin) };
    s_count++;
    return gpio_isr_handler_add(pin, button_isr, (void *)(uintptr_t)idx);
}
