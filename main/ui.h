#ifndef UI_H
#define UI_H
#include <stdint.h>

typedef enum { UI_WAITING, UI_LISTENING, UI_MODE_COUNT } ui_mode_t;

void ui_init(void);
void ui_set_mode(ui_mode_t m);
void ui_toggle_mode(void);
#endif
