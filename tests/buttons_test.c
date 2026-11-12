#include "button-task/button-task.h"
#include <assert.h>
static bool raw[2];
static unsigned count[2], long_count;
static uint64_t now;
uint64_t systime_us(void) { return now; }
bool platform_button_read(unsigned button) { return raw[button]; }
static void first(void) { ++count[0]; }
static void second(void) { ++count[1]; }
static void first_long(void) { ++long_count; }
static void advance(uint64_t us) { now+=us; button_task_handle(); }
static void release_first(void) {
    raw[0]=false; button_task_handle(); advance(20000);
}
int main(void) {
    button_task_init(0,first,first_long); button_task_init(1,second,0);
    raw[0]=true; button_task_handle(); advance(10000);
    raw[0]=false; button_task_handle(); advance(1000); /* press bounce */
    raw[0]=true; button_task_handle(); advance(20000);
    assert(!count[0] && !long_count && button_task_is_pressed(0));
    raw[1]=true; button_task_handle(); advance(20000); assert(count[1]==1 && !count[0]);
    assert(button_task_is_pressed(0) && button_task_is_pressed(1));
    raw[1]=false; button_task_handle(); now+=20000; button_task_handle();
    raw[1]=true; button_task_handle(); advance(20000); assert(count[1]==2 && !count[0]);
    /* Release bounce must not fire a short press until stable. */
    raw[0]=false; button_task_handle(); advance(5000);
    raw[0]=true; button_task_handle(); advance(5000); assert(!count[0]);
    release_first(); assert(count[0]==1 && !long_count);
    /* Exactly one long event at 1 second after debounced press. */
    raw[0]=true; button_task_handle(); advance(20000);
    advance(999999); assert(!long_count && count[0]==1);
    advance(1); assert(long_count==1 && count[0]==1);
    advance(5000000); assert(long_count==1 && count[1]==2);
    release_first(); assert(long_count==1 && count[0]==1);
    /* An ordinary short press works again after a long one. */
    raw[0]=true; button_task_handle(); advance(20000); advance(100000);
    release_first(); assert(count[0]==2 && long_count==1);
    /* Releasing just below the threshold is still a short press. */
    raw[0]=true; button_task_handle(); advance(20000); advance(999999);
    release_first(); assert(count[0]==3 && long_count==1);
    assert(button_task_get_press_count(0)==4 && button_task_get_press_count(1)==2);
    assert(!button_task_is_pressed(3) && button_task_get_press_count(3)==0);
}
