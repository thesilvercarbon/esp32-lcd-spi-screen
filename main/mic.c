#include "mic.h"
#include "driver/i2s_std.h"
#include "esp_check.h"

static const char *TAG = "mic";
static i2s_chan_handle_t rx;

esp_err_t mic_init(void){
    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan, NULL, &rx), TAG, "NEW CHANNEL");

    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_SCK_PIN,
            .ws = MIC_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = MIC_SD_PIN,
        },

    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &cfg), TAG, "init std");
    //return i2s_channel_enable(rx);
    return ESP_OK;
}

esp_err_t mic_start(void){ return i2s_channel_enable(rx); }
esp_err_t mic_stop(void){ return i2s_channel_disable(rx); }

esp_err_t mic_read(int32_t *buf, size_t n, size_t *got){
    size_t bytes = 0;
    esp_err_t err = i2s_channel_read(rx, buf, n * sizeof(int32_t), &bytes, 1000);
    *got = bytes / sizeof(int32_t);
    return err;
}



