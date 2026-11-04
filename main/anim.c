#include "anim.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_lvgl_port.h"

static const char *TAG = "anim";

typedef struct {
    const char *part;  uint16_t frames;
    const uint8_t *base;  esp_partition_mmap_handle_t h;
} clip_t;

static clip_t s_clips[ANIM_COUNT] = {
    [ANIM_LISTEN] = { "anim_listen", LISTEN_FRAMES },
    [ANIM_WAIT]   = { "anim_wait",   WAIT_FRAMES   },
};

static lv_image_dsc_t s_dsc;   // NOT const: the data pointer changes per frame
static lv_obj_t   *s_img;  static lv_timer_t *s_timer;
static clip_t     *s_cur;  static uint16_t s_idx;

static esp_err_t map_clip(clip_t *c)
{
    const esp_partition_t *p = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, c->part);
    if (!p) {
        ESP_LOGE(TAG, "partition '%s' not found", c->part);
        return ESP_ERR_NOT_FOUND;
    }
    if (p->size < (size_t)c->frames * ANIM_FRAME_BYTES) {
        ESP_LOGE(TAG, "partition '%s' too small: %lu bytes", c->part, (unsigned long)p->size);
        return ESP_ERR_INVALID_SIZE;
    }
    const void *ptr;
    esp_err_t err = esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &ptr, &c->h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mmap '%s' failed: %s", c->part, esp_err_to_name(err));
        return err;
    }
    c->base = ptr;
    ESP_LOGI(TAG, "'%s': %lu bytes, %u frames", c->part, (unsigned long)p->size, c->frames);
    return ESP_OK;
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_cur) return;
    s_dsc.data = s_cur->base + (size_t)s_idx * ANIM_FRAME_BYTES;
    lv_image_set_src(s_img, &s_dsc);               // invalidates -> redraw
    s_idx = (s_idx + ANIM_FRAME_STEP) % s_cur->frames;
}

esp_err_t anim_init(lv_obj_t *parent)
{
    for (int i = 0; i < ANIM_COUNT; i++) {
        esp_err_t err = map_clip(&s_clips[i]);
        if (err != ESP_OK) return err;
    }

    s_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    s_dsc.header.cf     = LV_COLOR_FORMAT_RGB565;
    s_dsc.header.w      = ANIM_W;
    s_dsc.header.h      = ANIM_H;
    s_dsc.header.stride = ANIM_W * 2;
    s_dsc.data_size     = ANIM_FRAME_BYTES;

    lvgl_port_lock(0);
    s_img = lv_image_create(parent);
    lv_obj_center(s_img);
    s_timer = lv_timer_create(tick_cb, ANIM_FRAME_MS, NULL);
    if (s_timer) lv_timer_pause(s_timer);
    lvgl_port_unlock();

    return (s_img && s_timer) ? ESP_OK : ESP_ERR_NO_MEM;
}

void anim_play(anim_id_t id)
{
    if (id >= ANIM_COUNT || !s_timer) return;
    lvgl_port_lock(0);
    if (s_cur != &s_clips[id]) {
        s_cur = &s_clips[id];
        s_idx = 0;
        lv_timer_resume(s_timer);
        lv_timer_ready(s_timer);                   // show frame 0 on the next LVGL cycle
    }
    lvgl_port_unlock();
}

void anim_stop(void)
{
    if (!s_timer) return;
    lvgl_port_lock(0);
    lv_timer_pause(s_timer);
    s_cur = NULL;                                  // so a later anim_play(same id) restarts
    lvgl_port_unlock();
}
