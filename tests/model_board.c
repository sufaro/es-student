/* Host platform model. The firmware parser, API and tasks are compiled unchanged.
 * Timing and analog signals below are model assumptions, never measurements. */
#include "platform/platform.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#if MODEL_LEVEL == 3
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"
#endif
static uint64_t now, boot;
static unsigned char input[4096];
static size_t input_at, input_size;
uint64_t time_us_64(void) { return now++ - boot; }
int getchar_timeout_us(unsigned timeout) {
    (void)timeout; return input_at<input_size ? input[input_at++] : -1;
}
void device_get_info(device_info_t *d) {
    *d=(device_info_t){.board="pico",.serial="0000000000000000",
        .manufacturer=0x4d,.part=0x2040,.revision=2,.sdk="MODEL"};
}
void platform_init(void) {}
void platform_led_set(bool on) { (void)on; }
bool platform_button_read(void) { return false; }
#if MODEL_LEVEL >= 2
static bool charge;
static double rc_value;
static uint64_t rc_time;
float platform_voltage_read(void) {
    double target=charge?3.3:0;
    return (float)(target+(rc_value-target)*exp(-(double)(now-rc_time)/1e6));
}
float platform_temp_read(void) { return 27.0f; }
void platform_charge_set(bool on) {
    rc_value=platform_voltage_read(); rc_time=now; charge=on;
}
#endif
#if MODEL_LEVEL == 3
static uint16_t samples[8192];
static uint32_t count, period, tone, wdt_ms, restarts;
static uint64_t finish_at, blink_at, stream_at, stream_index, wdt_at;
static bool capturing, block, streaming, hung, reset_wdt, blinking;
static unsigned bits, sequence;
static uint32_t blink_period;
/* MODEL signal: GP20 loopback sine; otherwise a synthetic 220 Hz microphone.
 * A silence mode is selected by the driver for the melody's gaps. */
static bool loopback;
static uint16_t signal_sample(uint64_t index) {
    unsigned hz=tone?tone:(loopback?0:220);
    double amplitude=hz? (loopback?1200:100) : 0;
    return (uint16_t)(1551+amplitude*sin(6.283185307179586*hz*index/44100.0));
}
bool platform_adc_busy(void) { return capturing || block || streaming; }
bool platform_timer_blink_start(uint32_t ms) {
    blinking=true; blink_at=now; blink_period=ms; return true;
}
uint32_t platform_timer_blink_stop(void) {
    uint32_t n=blinking?(uint32_t)((now-blink_at)/(blink_period*1000ull)):0;
    blinking=false; return n;
}
bool platform_capture_start(uint32_t n,uint32_t us) {
    count=n; period=us; finish_at=now+(uint64_t)n*us; capturing=true;
    for(uint32_t i=0;i<n;++i) samples[i]=signal_sample(i);
    return true;
}
bool platform_capture_done(void) { return capturing && now>=finish_at; }
void platform_capture_get(platform_capture_t *c) {
    *c=(platform_capture_t){samples,count,period,period,period,100.0f*2/period}; capturing=false;
}
bool platform_block_start(uint32_t n) {
    count=n; finish_at=now+(uint64_t)n*1000000/44100; block=true;
    for(uint32_t i=0;i<n;++i) samples[i]=signal_sample(i);
    return true;
}
bool platform_block_done(void) { return block && now>=finish_at; }
void platform_block_get(platform_block_t *b) {
    *b=(platform_block_t){samples,count,(uint32_t)((uint64_t)count*1000000/44100)}; block=false;
}
bool platform_stream_start(unsigned b) {
    bits=b; sequence=0; stream_index=0; stream_at=now; streaming=true; return true;
}
void platform_stream_stop(void) { streaming=false; }
bool platform_stream_take(uint8_t *data,uint16_t *seq) {
    unsigned n=bits==16?512:1024;
    if(!streaming || now-stream_at<(stream_index+n)*1000000/44100) return false;
    for(unsigned i=0;i<n;++i) {
        uint16_t v=signal_sample(stream_index++);
        if(bits==16) { data[2*i]=(uint8_t)v; data[2*i+1]=(uint8_t)(v>>8); }
        else data[i]=(uint8_t)(v>>4);
    }
    *seq=(uint16_t)sequence++; return true;
}
uint32_t platform_stream_overruns(void) { return 0; }
bool platform_frame_write(const void *data,size_t size) { return fwrite(data,1,size,stdout)==size; }
void platform_generate_tone(uint32_t hz) { tone=hz; loopback=true; }
void platform_watchdog_enable(uint32_t ms) { wdt_ms=ms; wdt_at=now; }
void platform_watchdog_update(void) { wdt_at=now; }
bool platform_reset_is_watchdog(void) { return reset_wdt; }
uint32_t platform_reset_count(void) { return restarts; }
void platform_hang(void) { hung=true; }
bool platform_is_hung(void) { return hung; }
#endif
static void init_tasks(void) {
    stdio_text_protocol_init(); led_task_init(); button_task_init(led_task_next_state); pi_task_init();
    const char *names[]={"loop","command","led","button","pi"};
    for(unsigned i=0;i<5;++i) profiling_stopwatch_init(i,names[i]);
#if MODEL_LEVEL >= 2
    telemetry_task_init(api_print_telemetry);
#endif
#if MODEL_LEVEL == 3
    block_task_init(api_print_block); stream_task_init(api_send_frame);
#endif
}
static void tick(void) {
#if MODEL_LEVEL == 3
    if(hung) {
        if(wdt_ms && now-wdt_at>=wdt_ms*1000ull) {
            boot=now; hung=false; wdt_ms=0; reset_wdt=true; ++restarts;
            capturing=block=streaming=blinking=false; init_tasks();
        }
        now+=150; return;
    }
#endif
    profiling_start(0); profiling_start(1);
    const command_t *c=stdio_text_protocol_handle(); if(c) api_handle(c);
    profiling_stop(1);
    profiling_start(2); led_task_handle(); profiling_stop(2);
    profiling_start(3); button_task_handle(); profiling_stop(3);
    profiling_start(4); pi_task_handle(); profiling_stop(4);
#if MODEL_LEVEL >= 2
    telemetry_task_handle();
#endif
#if MODEL_LEVEL == 3
    block_task_handle(); stream_task_handle(); platform_watchdog_update();
#endif
    profiling_stop(0); now+=150;
}
int main(void) {
#ifdef _WIN32
    _setmode(_fileno(stdout),_O_BINARY);
#endif
    init_tasks(); char line[16384];
    /* One line: virtual duration in us, then ASCII command encoded as hex.
     * Outputs exact firmware bytes followed by a transport-only separator. */
    while(fgets(line,sizeof(line),stdin)) {
        char *hex; uint64_t duration=strtoull(line,&hex,10); while(*hex==' ') ++hex;
        input_at=input_size=0;
        while(hex[0] && hex[1] && hex[0]!='\n') {
            unsigned v; if(sscanf(hex,"%2x",&v)!=1) break;
            input[input_size++]=(unsigned char)v; hex+=2;
        }
        uint64_t end=now+duration;
        while(now<end || input_at<input_size) tick();
        fputs("\n@MODEL-END\n",stdout); fflush(stdout);
    }
    return 0;
}
