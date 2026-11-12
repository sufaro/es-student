#pragma once
#include <stdbool.h>
#include <stdint.h>
#define STREAM_SYNC_WORD 0x5aa5
typedef struct { uint16_t sync,sequence,count; uint8_t samples[1024]; } stream_frame_t;
typedef struct { uint32_t frames,samples,overruns; } stream_stats_t;
typedef bool (*stream_task_callback_t)(const stream_frame_t *frame);
void stream_task_init(stream_task_callback_t callback);
bool stream_task_start(unsigned bits);
void stream_task_stop(stream_stats_t *stats);
void stream_task_handle(void);
bool stream_task_running(void);
