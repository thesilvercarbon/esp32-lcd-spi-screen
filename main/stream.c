#include "stream.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "lwip/sockets.h"

#define BUF_BYTES 16384
#define SEND_CHUNK 1024

static const char *TAG = "stream";
static StreamBufferHandle_t s_sb;
static TaskHandle_t s_task;
static volatile bool s_active;
static volatile size_t s_dropped;


static int connect_server(void){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in a = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_STREAM_PORT),
    };
    inet_pton(AF_INET, CONFIG_STREAM_HOST, &a.sin_addr);
    if (connect(fd, (struct sockaddr *)&a, sizeof a) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void stream_task(void *arg){
    uint8_t buf[SEND_CHUNK];
    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int fd = connect_server();
        if(fd < 0) ESP_LOGE(TAG, "connect failed (server running? firewall?)");
        size_t total =0;

        for(;;){
            bool last = !s_active;
            size_t n = xStreamBufferReceive(s_sb, buf, sizeof buf, pdMS_TO_TICKS(50));
            if (n == 0) { if (last) break; else continue; }
            if (fd >= 0){
                if (send(fd, buf, n, 0) < 0) {
                    ESP_LOGE(TAG, "send failed");
                    close(fd);
                    fd = -1;
                }
                else total += n;
            }
        }
        if (fd >= 0 ) close(fd);
        ESP_LOGI(TAG, "sent %u bytes dropped %u", (unsigned)total, (unsigned)s_dropped);
    }
}

esp_err_t stream_init(void){
    s_sb = xStreamBufferCreate(BUF_BYTES, 1);
    if (!s_sb) return ESP_ERR_NO_MEM;
    return xTaskCreate(stream_task, "stream", 6144, NULL, 4, &s_task) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}


void stream_begin(void) {s_dropped = 0, s_active = true; xTaskNotifyGive(s_task);}
void stream_end(void) {s_active = false;}

void stream_push(const int16_t *s, size_t n){
    size_t bytes = n * sizeof(int16_t);
    if (xStreamBufferSpacesAvailable(s_sb) < bytes){
        s_dropped += bytes;
        return;
    }
    xStreamBufferSend(s_sb, s, bytes, 0);
}









