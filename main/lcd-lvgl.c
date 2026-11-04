#include "lcd.h"
#include "ui.h"
#include "input.h"
#include "rec.h"
#include "stream.h"
#include "wifi_sta.h"

#include "mic.h"
#include "esp_log.h"
#include <stdlib.h>

static void on_toggle(bool pressed) { if (pressed) ui_toggle_mode(); }
static void on_hold(bool pressed)   {
    ui_set_mode(pressed ? UI_LISTENING : UI_WAITING); 
    if (pressed) rec_begin(); else rec_end();
}

void app_main(void)
{
    lcd_setup();
    ui_init();
    ESP_ERROR_CHECK(mic_init());
    ESP_ERROR_CHECK(stream_init());
    ESP_ERROR_CHECK(rec_init());
    if (wifi_start() != ESP_OK) ESP_LOGE("main", "continuing without Wi-Fi");
    ESP_ERROR_CHECK(input_init());                 // buttons last: no press before we're online
    ESP_ERROR_CHECK(input_add_button(BUTTON_PIN, on_toggle));
    if (HOLD_PIN != GPIO_NUM_NC) {
        ESP_ERROR_CHECK(input_add_button(HOLD_PIN, on_hold));
    }
   // ESP_ERROR_CHECK(mic_init());
   // static int32_t buf[240];
   // size_t got, blocks = 0;
   // int32_t peak = 0;
   // while (1) {
   //     esp_err_t err = mic_read(buf, 240, &got);
   //     if(err != ESP_OK){
   //         ESP_LOGE("mic", "read: %s", esp_err_to_name(err));
   //         continue;
   //     }

   //     for(size_t i=0; i < got; i++){
   //         int32_t s = abs(buf[i] >> 8);
   //         if(s > peak) peak = s;
   //     }
   //     if (++blocks % 50 == 0){
   //         ESP_LOGI("mic", "peak=%ld", (long)peak);
   //         peak = 0;
   //     }
   // }
}
