#include "stream-task/stream-task.h"
#include "block-task/block-task.h"
#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include <assert.h>
#include <string.h>

static bool busy, ready, accepting, block_done;
static unsigned sent, taken, records, blocks;
static uint64_t now;
bool platform_adc_busy(void) { return busy; }
bool platform_stream_start(unsigned bits) { if(busy || (bits!=8 && bits!=16)) return false; busy=true; return true; }
void platform_stream_stop(void) { busy=false; }
uint32_t platform_stream_overruns(void) { return 3; }
bool platform_stream_take(uint8_t *p,uint16_t *sequence) {
    if(!ready) return false;
    ready=false; memset(p,0x0a,1024); *sequence=65535; ++taken; return true;
}
bool platform_block_start(uint32_t count) { if(busy || !count || count>8192) return false; busy=true; block_done=false; return true; }
bool platform_block_done(void) { return block_done; }
void platform_block_get(platform_block_t *r) { static uint16_t samples[4]={1,2,3,4}; *r=(platform_block_t){samples,4,91}; }
float platform_voltage_read(void) { assert(!busy); return 1.65f; }
float platform_temp_read(void) { assert(!busy); return 27.0f; }
uint64_t systime_us(void) { return now; }
bool systime_period_elapsed(uint64_t *last,uint64_t period) { if(now-*last<period) return false; *last=now; return true; }
static bool send_frame(const stream_frame_t *f) {
    assert(f->sync==0x5aa5 && f->sequence==65535 && f->samples[0]==0x0a);
    if(!accepting) return false;
    ++sent; return true;
}
static void on_record(const telemetry_record_t *r) { assert(r->voltage_v==1.65f && r->temp_c==27); ++records; }
static void on_block(const block_result_t *b) { assert(b->count==4 && b->samples[3]==4 && b->kind==BLOCK_KIND_LEVEL && b->window_ms==100); ++blocks; }
int main(void) {
    stream_task_init(send_frame); assert(!stream_task_start(12)); assert(stream_task_start(16)); assert(!stream_task_start(16));
    ready=true; stream_task_handle(); assert(taken==1 && !sent);
    ready=true; stream_task_handle(); assert(taken==1); /* backpressure retains the pending frame */
    accepting=true; stream_task_handle(); assert(sent==1 && taken==1);
    stream_task_handle(); assert(sent==2 && taken==2);
    stream_stats_t stats; stream_task_stop(&stats); assert(!busy && stats.frames==2 && stats.samples==1024 && stats.overruns==3);
    assert(stream_task_start(8)); ready=true; stream_task_handle(); stream_task_stop(&stats); assert(stats.samples==1024 && stats.frames==1);
    block_task_init(on_block); assert(block_task_start(4,BLOCK_KIND_LEVEL,100)); assert(!block_task_start(4,BLOCK_KIND_LEVEL,100));
    block_task_handle(); assert(!blocks); block_done=true; busy=false; block_task_handle(); block_task_handle(); assert(blocks==1);
    telemetry_task_init(on_record); now=1000000; assert(!telemetry_task_start(9)); assert(telemetry_task_start(50));
    telemetry_task_handle(); assert(records==1); now+=49999; telemetry_task_handle(); assert(records==1);
    ++now; telemetry_task_handle(); assert(records==2);
    busy=true; now+=50000; telemetry_task_handle(); assert(records==2); busy=false;
    telemetry_task_stop(); telemetry_task_handle(); assert(records==2);
}
