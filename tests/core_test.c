#include "api/api.h"
#include "button-task/button-task.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static uint64_t now;
static bool raw_button, led;
static unsigned presses;
static const char *input;
uint64_t time_us_64(void) { return now; }
int getchar_timeout_us(unsigned timeout) { assert(timeout==0); return input && *input ? (unsigned char)*input++ : -1; }
bool platform_button_read(void) { return raw_button; }
void platform_led_set(bool on) { led=on; }
void device_get_info(device_info_t *info) { *info=(device_info_t){.board="TEST",.serial="0000000000000000",.sdk="TEST"}; }
static void on_press(void) { ++presses; }
static const command_t *parse(const char *text) {
    input=text;
    while(*input) { const command_t *c=stdio_text_protocol_handle(); if(c) return c; }
    return NULL;
}
static void protocol_tests(void) {
    stdio_text_protocol_init();
    const command_t *c=parse("  led_period   2000  \r");
    assert(c && !strcmp(c->name,"led_period") && c->argc==1 && !strcmp(c->argv[0],"2000") && !c->truncated);
    assert(!parse("\n"));
    c=parse("hepl\b\blp\n"); assert(c && !strcmp(c->name,"help"));
    c=parse("abc\177d\n"); assert(c && !strcmp(c->name,"abd"));
    assert(!parse("   \r"));
    c=parse("a b c d e f\n"); assert(c && c->truncated);
    char text[80]; memset(text,'x',sizeof(text)); text[64]='\n'; text[65]=0;
    c=parse(text); assert(c && c->truncated);
    text[64]='\b'; text[65]='\n'; text[66]=0;
    c=parse(text); assert(c && !c->truncated && strlen(c->name)==62);
    text[63]='\n'; text[64]=0; c=parse(text); assert(c && !c->truncated && strlen(c->name)==63);
    assert(!parse("partial")); assert(!stdio_text_protocol_handle());
    c=parse(" command\n"); assert(c && !strcmp(c->name,"partial") && c->argc==1);
}
static void task_tests(void) {
    now=0; led_task_init(); assert(!led && led_task_get_state()==LED_STATE_BLINK);
    now=500000; led_task_handle(); assert(led);
    now=1000030; led_task_handle(); assert(!led);
    now=1500000; led_task_handle(); assert(led); /* no accumulated drift */
    now=9000000; led_task_handle(); assert(!led);
    now=9000001; led_task_handle(); assert(!led); /* no catch-up burst */
    assert(led_task_set_period_ms(UINT32_MAX)); assert(!led_task_set_period_ms(0));
    led_task_set_state(LED_STATE_OFF); led_task_next_state(); assert(led);
    led_task_next_state(); assert(led_task_get_state()==LED_STATE_BLINK);
    raw_button=false; button_task_init(on_press);
    raw_button=true; button_task_handle(); now+=10000; raw_button=false; button_task_handle();
    now+=1000; raw_button=true; button_task_handle(); now+=19999; button_task_handle(); assert(!presses);
    now+=1; button_task_handle(); assert(presses==1 && button_task_is_pressed());
    now+=2000000; button_task_handle(); assert(presses==1);
    raw_button=false; button_task_handle(); now+=20000; button_task_handle(); assert(!button_task_is_pressed());
    raw_button=true; button_task_handle(); now+=20000; button_task_handle(); assert(presses==2 && button_task_get_press_count()==2);
    pi_task_init(); pi_result_t r; pi_task_get_result(&r); assert(r.state==PI_STATE_IDLE);
    assert(!pi_task_start(0)); assert(pi_task_start(1000000)); assert(!pi_task_start(1));
    pi_task_handle(); pi_task_get_result(&r); assert(r.state==PI_STATE_RUN);
    do { ++now; pi_task_handle(); pi_task_get_result(&r); } while(r.state==PI_STATE_RUN);
    assert(fabs(r.value-3.141592653589793)<1.01e-6);
    assert(pi_task_start(UINT32_MAX)); pi_task_handle(); pi_task_get_result(&r); assert(r.state==PI_STATE_RUN);
    uint64_t last=100; now=209; assert(systime_period_elapsed(&last,100) && last==200);
    now=800; assert(systime_period_elapsed(&last,100) && last==800);
    now=899; assert(!systime_period_elapsed(&last,100));
    profiling_stopwatch_init(0,"test"); now=100; profiling_start(0); now=130; profiling_stop(0);
    profiling_start(0); now=140; profiling_stop(0); profiling_result_t pr; assert(profiling_get(0,&pr));
    assert(pr.count==2 && pr.mean_us==20 && pr.max_us==30); profiling_reset(); assert(profiling_get(0,&pr) && pr.count==0);
}
static void api_tests(void) {
    led_task_init();
    const char *invalid[]={"-1","+1","0x10","5x","4294967296","99999999999999999999999999999","0"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) {
        command_t c={.name="led_period",.argc=1,.argv={invalid[i]}}; api_handle(&c); assert(led_task_get_period_ms()==1000);
    }
    command_t c={.name="led_period",.argc=1,.argv={"4294967295"}};
    api_handle(&c); assert(led_task_get_period_ms()==UINT32_MAX);
    c.argv[0]="10"; c.truncated=true; api_handle(&c); assert(led_task_get_period_ms()==UINT32_MAX);
}
int main(void) { protocol_tests(); task_tests(); api_tests(); puts("core tests passed"); }
