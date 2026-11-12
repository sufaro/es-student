#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif
#define PROFILING_MAX_STOPWATCHES 12
typedef struct { const char *name; uint64_t count; double mean_us; uint32_t max_us; } profiling_result_t;
#if PROFILING_ENABLED
void profiling_stopwatch_init(unsigned id, const char *name);
void profiling_start(unsigned id);
void profiling_stop(unsigned id);
bool profiling_get(unsigned id, profiling_result_t *result);
void profiling_reset(void);
#else
#define profiling_stopwatch_init(id,name) ((void)0)
#define profiling_start(id) ((void)0)
#define profiling_stop(id) ((void)0)
#define profiling_get(id,result) (false)
#define profiling_reset() ((void)0)
#endif
