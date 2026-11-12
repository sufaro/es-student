#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
typedef struct { bool pressed, candidate; uint64_t changed; uint32_t count; button_task_callback_t callback; } button_t;
static button_t buttons[2];
void button_task_init(unsigned button,button_task_callback_t cb) { if(button<2) buttons[button]=(button_t){.callback=cb}; }
void button_task_handle(void) {
    for(unsigned i=0;i<2;++i) {
        button_t *b=&buttons[i]; bool raw=platform_button_read(i);
        if(raw!=b->candidate) { b->candidate=raw; b->changed=systime_us(); }
        if(raw!=b->pressed && systime_us()-b->changed>=20000) {
            b->pressed=raw; if(raw) { ++b->count; if(b->callback) b->callback(); }
        }
    }
}
bool button_task_is_pressed(unsigned button) { return button<2 && buttons[button].pressed; }
uint32_t button_task_get_press_count(unsigned button) { return button<2?buttons[button].count:0; }
