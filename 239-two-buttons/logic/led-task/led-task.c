#include "led-task/led-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
static led_state_t state;
static uint32_t period_ms;
static uint64_t last_us;
static bool on, suspended;
void led_task_set_state(led_state_t next) {
    state=next; on=(state==LED_STATE_ON); last_us=systime_us(); platform_led_set(on);
}
void led_task_init(void) { period_ms=1000; suspended=false; led_task_set_state(LED_STATE_BLINK); }
void led_task_handle(void) {
    if(suspended) return;
    switch(state) {
    case LED_STATE_BLINK:
        if(systime_period_elapsed(&last_us,(uint64_t)period_ms*1000/2)) { on=!on; platform_led_set(on); } break;
    case LED_STATE_ON: case LED_STATE_OFF: break;
    }
}
led_state_t led_task_get_state(void) { return state; }
bool led_task_set_period_ms(uint32_t value) { if(!value) return false; period_ms=value; last_us=systime_us(); return true; }
uint32_t led_task_get_period_ms(void) { return period_ms; }
void led_task_next_state(void) { led_task_set_state((led_state_t)((state+1)%3)); }
void led_task_suspend(bool value) { suspended=value; if(!value) led_task_set_state(state); }

void led_task_next_period(void) { uint32_t p=led_task_get_period_ms(); led_task_set_period_ms(p==250?500:p==500?1000:250); }
