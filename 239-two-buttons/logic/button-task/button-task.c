#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
enum { DEBOUNCE_US=20000, LONG_PRESS_US=1000000 };
typedef enum { RELEASED, HELD, LONG_SENT } button_state_t;
typedef struct {
    bool pressed, candidate;
    uint64_t changed, pressed_at;
    uint32_t count;
    button_state_t state;
    button_task_callback_t callback, long_callback;
} button_t;
static button_t buttons[2];
void button_task_init(unsigned button,button_task_callback_t cb,button_task_callback_t long_cb) {
    if(button<2) buttons[button]=(button_t){.callback=cb,.long_callback=long_cb};
}
void button_task_handle(void) {
    for(unsigned i=0;i<2;++i) {
        button_t *b=&buttons[i]; bool raw=platform_button_read(i);
        uint64_t now=systime_us();
        if(raw!=b->candidate) { b->candidate=raw; b->changed=now; }
        if(raw!=b->pressed && now-b->changed>=DEBOUNCE_US) {
            b->pressed=raw;
        }
        switch(b->state) {
        case RELEASED:
            if(b->pressed) {
                ++b->count; b->pressed_at=now; b->state=HELD;
                /* With no long action, keep the original press event. */
                if(!b->long_callback && b->callback) b->callback();
            }
            break;
        case HELD:
            if(!b->pressed) {
                b->state=RELEASED;
                if(b->long_callback && b->callback) b->callback();
            } else if(b->long_callback && raw && now-b->pressed_at>=LONG_PRESS_US) {
                b->state=LONG_SENT; b->long_callback();
            }
            break;
        case LONG_SENT:
            if(!b->pressed) b->state=RELEASED;
            break;
        }
    }
}
bool button_task_is_pressed(unsigned button) { return button<2 && buttons[button].pressed; }
uint32_t button_task_get_press_count(unsigned button) { return button<2?buttons[button].count:0; }
