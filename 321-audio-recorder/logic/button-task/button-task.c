#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
typedef enum { BUTTON_STABLE, BUTTON_DEBOUNCE } button_state_t;
static button_state_t state;
static bool pressed, candidate;
static uint64_t changed_us;
static uint32_t presses;
static button_task_callback_t callback;
void button_task_init(button_task_callback_t on_press) { callback=on_press; state=BUTTON_STABLE; pressed=false; presses=0; }
void button_task_handle(void) {
    bool raw=platform_button_read();
    switch(state) {
    case BUTTON_STABLE:
        if(raw!=pressed) { candidate=raw; changed_us=systime_us(); state=BUTTON_DEBOUNCE; } break;
    case BUTTON_DEBOUNCE:
        if(raw!=candidate) { candidate=raw; changed_us=systime_us(); }
        if(systime_us()-changed_us>=20000) {
            state=BUTTON_STABLE;
            if(pressed!=candidate) { pressed=candidate; if(pressed) { ++presses; if(callback) callback(); } }
        } break;
    }
}
bool button_task_is_pressed(void) { return pressed; }
uint32_t button_task_get_press_count(void) { return presses; }
