#include "button-task/button-task.h"
#include <assert.h>
static bool raw[2];
static unsigned count[2];
static uint64_t now;
uint64_t systime_us(void) { return now; }
bool platform_button_read(unsigned button) { return raw[button]; }
static void first(void) { ++count[0]; }
static void second(void) { ++count[1]; }
int main(void) {
    button_task_init(0,first); button_task_init(1,second);
    raw[0]=true; button_task_handle(); now=20000; button_task_handle(); assert(count[0]==1 && !count[1]);
    raw[1]=true; button_task_handle(); now+=20000; button_task_handle(); assert(count[1]==1 && count[0]==1);
    assert(button_task_is_pressed(0) && button_task_is_pressed(1));
    raw[1]=false; button_task_handle(); now+=20000; button_task_handle();
    raw[1]=true; button_task_handle(); now+=20000; button_task_handle(); assert(count[1]==2 && count[0]==1);
    assert(button_task_get_press_count(0)==1 && button_task_get_press_count(1)==2);
}
