#include "block-task/block-task.h"
#include "platform/platform.h"
static bool running;
static block_kind_t kind;
static uint32_t window;
static block_task_callback_t callback;
void block_task_init(block_task_callback_t cb) { callback=cb; running=false; }
bool block_task_start(uint32_t count,block_kind_t mode,uint32_t ms) {
    if(running || !platform_block_start(count)) return false;
    running=true; kind=mode; window=ms; return true;
}
void block_task_handle(void) {
    if(running && platform_block_done()) {
        platform_block_t b; platform_block_get(&b); running=false;
        block_result_t r={b.samples,b.count,b.elapsed_us,window,kind}; if(callback) callback(&r);
    }
}
