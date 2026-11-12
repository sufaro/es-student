#include "stream-task/stream-task.h"
#include "platform/platform.h"
static bool running,pending;
static stream_frame_t frame;
static stream_stats_t totals;
static stream_task_callback_t callback;
_Static_assert(sizeof(stream_frame_t)==1030,"stream wire format");
void stream_task_init(stream_task_callback_t cb) { callback=cb; running=false; }
bool stream_task_start(unsigned bits) {
    if(running || !platform_stream_start(bits)) return false;
    frame.sync=STREAM_SYNC_WORD; frame.count=bits==16?512:1024; totals=(stream_stats_t){0}; pending=false; running=true; return true;
}
void stream_task_stop(stream_stats_t *stats) {
    if(running) { platform_stream_stop(); totals.overruns=platform_stream_overruns(); if(pending) ++totals.overruns; }
    running=pending=false; *stats=totals;
}
void stream_task_handle(void) {
    if(!running) return;
    if(!pending) pending=platform_stream_take(frame.samples,&frame.sequence);
    if(pending && callback && callback(&frame)) { ++totals.frames; totals.samples+=frame.count; pending=false; }
}
bool stream_task_running(void) { return running; }
