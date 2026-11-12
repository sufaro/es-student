#include "api/api-commands.h"
#include "api/api.h"
#include "firmware.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static bool parse_u32(const char *text,uint32_t *value) {
    if(!text || !*text) return false;
    for(const char *p=text;*p;++p) if(*p<'0'||*p>'9') return false;
    errno=0; char *end; unsigned long v=strtoul(text,&end,10);
    if(errno || *end || v>UINT32_MAX) return false;
    *value=(uint32_t)v; return true;
}
static void cmd_info(const command_t *c) {
    (void)c; device_info_t d; device_get_info(&d);
    printf("name: %s\nversion: %s\nproject: %s\nrepo: %s\nboard: %s\nserial: %s\nchip: manufacturer 0x%03lx, part 0x%04lx, revision %lu\npico-sdk: %s\n",
        FIRMWARE_NAME,FIRMWARE_VERSION,FIRMWARE_PROJECT,FIRMWARE_REPO,d.board,d.serial,(unsigned long)d.manufacturer,(unsigned long)d.part,(unsigned long)d.revision,d.sdk);
}
static void cmd_uptime(const command_t *c) { (void)c; printf("uptime: %llu ms\n",(unsigned long long)systime_ms()); }
static void print_led(void) { const char *names[]={"off","on","blink"}; printf("led: %s, period %lu ms\n",names[led_task_get_state()],(unsigned long)led_task_get_period_ms()); }
static void cmd_led_enable(const command_t *c) { (void)c; led_task_set_state(LED_STATE_ON); print_led(); }
static void cmd_led_disable(const command_t *c) { (void)c; led_task_set_state(LED_STATE_OFF); print_led(); }
static void cmd_led_blink(const command_t *c) { (void)c; led_task_set_state(LED_STATE_BLINK); print_led(); }
static void cmd_led_period(const command_t *c) {
    uint32_t v;
    if(c->argc!=1 || !parse_u32(c->argv[0],&v)) { puts("error: usage led_period <period_ms>"); return; }
    if(!led_task_set_period_ms(v)) { puts("error: period_ms must be greater than 0"); return; } print_led();
}
static void cmd_button(const command_t *c) { (void)c; for(unsigned i=0;i<2;++i) printf("button %u: %s, presses %lu\n",i+1,button_task_is_pressed(i)?"pressed":"released",(unsigned long)button_task_get_press_count(i)); }
static void cmd_pi_start(const command_t *c) {
    uint32_t n=1000000;
    if(c->argc>1 || (c->argc && !parse_u32(c->argv[0],&n))) { puts("error: usage pi_start [<terms>]"); return; }
    if(!n) { puts("error: terms must be greater than 0"); return; }
    if(!pi_task_start(n)) { puts("error: pi is busy"); return; }
    printf("pi: started, %lu terms\n",(unsigned long)n);
}
static void cmd_pi(const command_t *c) { (void)c; pi_result_t r; pi_task_get_result(&r);
    if(r.state==PI_STATE_IDLE) puts("pi: not started");
    else if(r.state==PI_STATE_RUN) puts("pi: running");
    else printf("pi: %.9f in %llu ms\n",r.value,(unsigned long long)r.time_ms);
}
static void cmd_profiling(const command_t *c) {
    (void)c;
#if PROFILING_ENABLED
    profiling_result_t r;
    for(unsigned i=0;profiling_get(i,&r);++i) printf("%-10s count %llu  mean %.2f us  max %lu us\n",r.name,(unsigned long long)r.count,r.mean_us,(unsigned long)r.max_us);
#else
    puts("profiling is disabled in this build");
#endif
}
static void cmd_profiling_reset(const command_t *c) { (void)c; profiling_reset(); puts("profiling reset"); }
/* EXTRA_CALLBACKS */
const api_command_t api_commands[]={
    {"info","device passport",cmd_info}, {"uptime","time since reset",cmd_uptime},
    {"led_enable","LED on",cmd_led_enable}, {"led_disable","LED off",cmd_led_disable}, {"led_blink","LED blink",cmd_led_blink},
    {"led_period","LED period in ms",cmd_led_period}, {"button","button state and press count",cmd_button},
    {"pi_start","calculate pi: pi_start [terms]",cmd_pi_start}, {"pi","pi result",cmd_pi},
    {"profiling","stopwatch statistics",cmd_profiling}, {"profiling_reset","reset stopwatches",cmd_profiling_reset},
    /* EXTRA_COMMANDS */
};
const size_t api_command_count=sizeof(api_commands)/sizeof(api_commands[0]);
