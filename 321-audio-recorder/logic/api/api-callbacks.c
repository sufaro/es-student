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
static void cmd_button(const command_t *c) { (void)c; printf("button: %s, presses %lu\n",button_task_is_pressed()?"pressed":"released",(unsigned long)button_task_get_press_count()); }
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
#include "platform/platform.h"
void api_print_telemetry(const telemetry_record_t *r) { printf("tm: %llu %.3f %.1f\n",(unsigned long long)r->time_us,r->voltage_v,r->temp_c); }
static void cmd_get_voltage(const command_t *c) { (void)c; if(platform_adc_busy()) { puts("error: adc busy"); return; } printf("voltage: %.3f V\n",platform_voltage_read()); }
static void cmd_get_temp(const command_t *c) { (void)c; if(platform_adc_busy()) { puts("error: adc busy"); return; } printf("temp: %.1f C\n",platform_temp_read()); }
static void cmd_tm_start(const command_t *c) {
    uint32_t ms=100;
    if(c->argc>1 || (c->argc && !parse_u32(c->argv[0],&ms))) { puts("error: usage tm_start [<period_ms>]"); return; }
    if(platform_adc_busy()) { puts("error: adc busy"); return; }
    if(!telemetry_task_start(ms)) { puts("error: period_ms must be at least 10"); return; }
    printf("telemetry: started, period %lu ms\n",(unsigned long)ms);
}
static void cmd_tm_stop(const command_t *c) { (void)c; telemetry_task_stop(); puts("telemetry: stopped"); }
static void cmd_charge_on(const command_t *c) { (void)c; platform_charge_set(true); puts("charge: on"); }
static void cmd_charge_off(const command_t *c) { (void)c; platform_charge_set(false); puts("charge: off"); }

#include <math.h>
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"
static bool one_number(const command_t *c,uint32_t *v,const char *usage) {
    if(c->argc!=1 || !parse_u32(c->argv[0],v)) { printf("error: usage %s\n",usage); return false; } return true;
}
static bool require_adc_idle(void) { if(platform_adc_busy()) { puts("error: adc busy"); return false; } return true; }
bool api_send_frame(const stream_frame_t *frame) { return platform_frame_write(frame,sizeof(*frame)); }
void api_print_block(const block_result_t *b) {
    uint16_t lo=4095,hi=0; double sum=0,squares=0;
    for(uint32_t i=0;i<b->count;++i) { double v=b->samples[i]; if(v<lo) lo=(uint16_t)v; if(v>hi) hi=(uint16_t)v; sum+=v; squares+=v*v; }
    double mean=sum/b->count;
    if(b->kind==BLOCK_KIND_BIAS) printf("mic: bias %.3f V\n",mean*3.3/4096);
    else if(b->kind==BLOCK_KIND_LEVEL) {
        double variance=squares/b->count-mean*mean;
        double peak=fmax(hi-mean,mean-lo);
        printf("mic: level %.1f mV rms, peak %.1f mV, %lu ms\n",sqrt(fmax(variance,0))*3300/4096,peak*3300/4096,(unsigned long)b->window_ms);
    } else printf("dma_cap: %lu samples in %lu us, expected %lu us, min %u max %u mean %.1f\n",(unsigned long)b->count,(unsigned long)b->elapsed_us,(unsigned long)((uint64_t)b->count*1000000/44100),lo,hi,mean);
}
static void cmd_timer_blink_start(const command_t *c) {
    if(stream_task_running()) { puts("error: stream is running"); return; }
    uint32_t ms; if(!one_number(c,&ms,"timer_blink_start <period_ms>")) return;
    if(ms<10||ms>60000) { puts("error: period_ms must be 10..60000"); return; }
    if(platform_timer_blink_start(ms)) printf("timer_blink: started, period %lu ms\n",(unsigned long)ms);
    else puts("error: timer unavailable");
}
static void cmd_timer_blink_stop(const command_t *c) { (void)c; if(stream_task_running()) { puts("error: stream is running"); return; } printf("timer_blink: stopped, %lu toggles\n",(unsigned long)platform_timer_blink_stop()); }
static void cmd_cap_start(const command_t *c) {
    uint32_t n,us;
    if(c->argc!=2 || !parse_u32(c->argv[0],&n) || !parse_u32(c->argv[1],&us)) { puts("error: usage cap_start <N> <period_us>"); return; }
    if(!n || n>2000) { puts("error: N must be 1..2000"); return; }
    if(us<10 || us>INT32_MAX) { puts("error: period_us must be at least 10"); return; }
    if(!require_adc_idle()) return;
    if(platform_capture_start(n,us)) printf("capture: started, %lu samples, period %lu us\n",(unsigned long)n,(unsigned long)us);
    else puts("error: timer unavailable");
}
static void cmd_cap_dump(const command_t *c) {
    (void)c; if(!platform_capture_done()) { puts("error: capture not ready"); return; }
    platform_capture_t b; platform_capture_get(&b);
    printf("capture: %lu samples, period %lu us, interval %lu..%lu us, handler %.1f %%\n",(unsigned long)b.count,(unsigned long)b.period_us,(unsigned long)b.min_interval_us,(unsigned long)b.max_interval_us,b.handler_percent);
    for(uint32_t i=0;i<b.count;++i) { if(i%16==0) printf("cap:"); printf(" %u",b.samples[i]); if(i%16==15 || i+1==b.count) puts(""); }
}
static void cmd_dma_cap(const command_t *c) {
    uint32_t n; if(!one_number(c,&n,"dma_cap <N>")) return;
    if(!n || n>8192) { puts("error: N must be 1..8192"); return; }
    if(require_adc_idle() && !block_task_start(n,BLOCK_KIND_DMA,0)) puts("error: adc busy");
}
static void cmd_generate_tone(const command_t *c) {
    uint32_t hz; if(!one_number(c,&hz,"generate_tone <Hz>")) return;
    if(hz && (hz<20||hz>20000)) { puts("error: Hz must be 0 or 20..20000"); return; }
    platform_generate_tone(hz); if(hz) printf("tone: %lu Hz\n",(unsigned long)hz); else puts("tone: off");
}
static void cmd_mic_bias(const command_t *c) { (void)c; if(require_adc_idle() && !block_task_start(4410,BLOCK_KIND_BIAS,100)) puts("error: adc busy"); }
static void cmd_mic_level(const command_t *c) {
    uint32_t ms=100;
    if(c->argc>1 || (c->argc && !parse_u32(c->argv[0],&ms))) { puts("error: usage mic_level [<ms>]"); return; }
    if(!ms||ms>185) { puts("error: ms must be 1..185"); return; }
    if(require_adc_idle() && !block_task_start(ms*44100/1000,BLOCK_KIND_LEVEL,ms)) puts("error: adc busy");
}
static void cmd_stream_start(const command_t *c) {
    uint32_t bits=16;
    if(c->argc>1 || (c->argc && !parse_u32(c->argv[0],&bits))) { puts("error: usage stream_start [8|16]"); return; }
    if(bits!=8 && bits!=16) { puts("error: bits must be 8 or 16"); return; }
    if(!require_adc_idle()) return;
    telemetry_task_stop();
    if(stream_task_start(bits)) { platform_timer_blink_start(250); printf("stream: started, 44100 Hz, %lu bit\n",(unsigned long)bits); }
    else puts("error: adc busy");
}
static void cmd_stream_stop(const command_t *c) {
    (void)c; stream_stats_t s; stream_task_stop(&s); platform_timer_blink_stop();
    printf("stream: stopped, %lu frames, %lu samples, %lu overruns\n",(unsigned long)s.frames,(unsigned long)s.samples,(unsigned long)s.overruns);
}
static void cmd_wdt_enable(const command_t *c) {
    uint32_t ms; if(!one_number(c,&ms,"wdt_enable <timeout_ms>")) return;
    if(!ms||ms>8388) { puts("error: timeout_ms must be 1..8388"); return; }
    platform_watchdog_enable(ms); printf("wdt: enabled, timeout %lu ms\n",(unsigned long)ms);
}
static void cmd_hang(const command_t *c) { (void)c; puts("hang: superloop stopped"); platform_hang(); }
static void cmd_reset_reason(const command_t *c) { (void)c; printf("reset: %s, restarts %lu\n",platform_reset_is_watchdog()?"watchdog":"other",(unsigned long)platform_reset_count()); }

const api_command_t api_commands[]={
    {"info","device passport",cmd_info}, {"uptime","time since reset",cmd_uptime},
    {"led_enable","LED on",cmd_led_enable}, {"led_disable","LED off",cmd_led_disable}, {"led_blink","LED blink",cmd_led_blink},
    {"led_period","LED period in ms",cmd_led_period}, {"button","button state and press count",cmd_button},
    {"pi_start","calculate pi: pi_start [terms]",cmd_pi_start}, {"pi","pi result",cmd_pi},
    {"profiling","stopwatch statistics",cmd_profiling}, {"profiling_reset","reset stopwatches",cmd_profiling_reset},
    {"get_voltage","GP26 voltage",cmd_get_voltage}, {"get_temp","chip temperature",cmd_get_temp},
    {"tm_start","telemetry: tm_start [period_ms]",cmd_tm_start}, {"tm_stop","stop telemetry",cmd_tm_stop},
    {"charge_on","charge RC circuit on GP22",cmd_charge_on}, {"charge_off","discharge RC circuit",cmd_charge_off},
    
    {"timer_blink_start","timer LED period in ms",cmd_timer_blink_start}, {"timer_blink_stop","stop timer LED",cmd_timer_blink_stop},
    {"cap_start","timer capture: cap_start N period_us",cmd_cap_start}, {"cap_dump","timer capture results",cmd_cap_dump},
    {"dma_cap","capture N samples using DMA",cmd_dma_cap}, {"generate_tone","PWM tone frequency in Hz; 0 stops",cmd_generate_tone},
    {"mic_bias","microphone DC bias",cmd_mic_bias}, {"mic_level","microphone level: mic_level [ms]",cmd_mic_level},
    {"stream_start","stream_start [8|16]",cmd_stream_start}, {"stream_stop","stop stream",cmd_stream_stop},
    {"wdt_enable","watchdog timeout in ms",cmd_wdt_enable}, {"hang","stop superloop until reset",cmd_hang}, {"reset_reason","reset reason and count",cmd_reset_reason},
};
const size_t api_command_count=sizeof(api_commands)/sizeof(api_commands[0]);
