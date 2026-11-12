#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum { BLOCK_KIND_DMA, BLOCK_KIND_BIAS, BLOCK_KIND_LEVEL } block_kind_t;
typedef struct { const uint16_t *samples; uint32_t count,elapsed_us,window_ms; block_kind_t kind; } block_result_t;
typedef void (*block_task_callback_t)(const block_result_t *block);
void block_task_init(block_task_callback_t callback);
bool block_task_start(uint32_t count,block_kind_t kind,uint32_t window_ms);
void block_task_handle(void);
