#include "firmware.h"
#include "platform/platform.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "profiling/profiling.h"
#include "logging/log.h"
#include <stddef.h>
enum { STOPWATCH_LOOP, STOPWATCH_COMMAND, STOPWATCH_LED, STOPWATCH_BUTTON, STOPWATCH_PI, STOPWATCH_TELEMETRY, STOPWATCH_BLOCK, STOPWATCH_STREAM };
int main(void) {
    platform_init(); stdio_text_protocol_init(); led_task_init(); button_task_init(led_task_next_state); pi_task_init();
    profiling_stopwatch_init(STOPWATCH_LOOP,"loop"); profiling_stopwatch_init(STOPWATCH_COMMAND,"command");
    profiling_stopwatch_init(STOPWATCH_LED,"led"); profiling_stopwatch_init(STOPWATCH_BUTTON,"button"); profiling_stopwatch_init(STOPWATCH_PI,"pi");
    telemetry_task_init(api_print_telemetry); profiling_stopwatch_init(STOPWATCH_TELEMETRY,"telemetry");
    block_task_init(api_print_block); stream_task_init(api_send_frame);
    profiling_stopwatch_init(STOPWATCH_BLOCK,"block"); profiling_stopwatch_init(STOPWATCH_STREAM,"stream");
    LOG_INF("%s %s",FIRMWARE_NAME,FIRMWARE_VERSION);
    while(1) {
        if(platform_is_hung()) continue;
        profiling_start(STOPWATCH_LOOP);
        profiling_start(STOPWATCH_COMMAND);
        const command_t *command=stdio_text_protocol_handle(); if(command!=NULL) api_handle(command);
        profiling_stop(STOPWATCH_COMMAND);
        profiling_start(STOPWATCH_LED); led_task_handle(); profiling_stop(STOPWATCH_LED);
        profiling_start(STOPWATCH_BUTTON); button_task_handle(); profiling_stop(STOPWATCH_BUTTON);
        profiling_start(STOPWATCH_PI); pi_task_handle(); profiling_stop(STOPWATCH_PI);
        profiling_start(STOPWATCH_TELEMETRY); telemetry_task_handle(); profiling_stop(STOPWATCH_TELEMETRY);
        profiling_start(STOPWATCH_BLOCK); block_task_handle(); profiling_stop(STOPWATCH_BLOCK);
        profiling_start(STOPWATCH_STREAM); stream_task_handle(); profiling_stop(STOPWATCH_STREAM);
        platform_watchdog_update();
        profiling_stop(STOPWATCH_LOOP);
    }
}
