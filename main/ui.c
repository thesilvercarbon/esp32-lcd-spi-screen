#include "ui.h"
#include "lcd.h"
#include "anim.h"
#include "esp_check.h"

static lv_obj_t *s_anim_box, *s_label, *s_text_box;
static ui_mode_t s_mode = UI_MODE_COUNT;     // "nothing shown yet", so the first apply always runs

static const struct { const char *text; uint32_t bg; anim_id_t clip; } MODE[UI_MODE_COUNT] = {
    [UI_WAITING]   = { "DEX\nwaiting\n...",   0x000000, ANIM_WAIT   },
    [UI_LISTENING] = { "DEX\nlistening\n...", 0x00FF00, ANIM_LISTEN },
};

static void make_flat(lv_obj_t *o)
{
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

// Caller holds the LVGL lock.
static void apply_mode(ui_mode_t m)
{
    lv_label_set_text(s_label, MODE[m].text);
    lv_obj_set_style_bg_color(s_text_box, lv_color_hex(MODE[m].bg), 0);
    anim_play(MODE[m].clip);
    s_mode = m;
}

void ui_init(void)
{
    lvgl_port_lock(0);
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    s_anim_box = lv_obj_create(scr);
    s_text_box = lv_obj_create(scr);
    make_flat(s_anim_box);
    make_flat(s_text_box);
    lv_obj_set_size(s_anim_box, ANIM_W, ANIM_H);
    lv_obj_set_size(s_text_box, LCD_WIDTH, LCD_HEIGHT - ANIM_H);
    lv_obj_align(s_anim_box, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_align_to(s_text_box, s_anim_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_anim_box, lv_color_black(), 0);

    s_label = lv_label_create(s_text_box);
    lv_obj_set_width(s_label, LV_PCT(100));
    lv_label_set_long_mode(s_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_font(s_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(s_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 0);

    ESP_ERROR_CHECK(anim_init(s_anim_box));     // recursive lock: safe while we hold it
    apply_mode(UI_WAITING);
    lvgl_port_unlock();
}

void ui_set_mode(ui_mode_t m)
{
    if ((unsigned)m >= UI_MODE_COUNT) return;
    lvgl_port_lock(0);
    if (m != s_mode) apply_mode(m);
    lvgl_port_unlock();
}

void ui_toggle_mode(void){
    lvgl_port_lock(0);
    ui_mode_t next = (s_mode == UI_WAITING) ? UI_LISTENING : UI_WAITING;
    apply_mode(next);
    lvgl_port_unlock();
}





