#include "profiling/profiling.h"
#include "pico/time.h"
#if PROFILING_ENABLED
typedef struct { const char *name; uint64_t start, total, count; uint32_t max; } stopwatch_t;
static stopwatch_t watches[PROFILING_MAX_STOPWATCHES];
void profiling_stopwatch_init(unsigned id,const char *name) { if(id<PROFILING_MAX_STOPWATCHES) watches[id]=(stopwatch_t){.name=name}; }
void profiling_start(unsigned id) { if(id<PROFILING_MAX_STOPWATCHES) watches[id].start=time_us_64(); }
void profiling_stop(unsigned id) {
    if(id>=PROFILING_MAX_STOPWATCHES) return;
    stopwatch_t *w=&watches[id]; uint32_t dt=(uint32_t)(time_us_64()-w->start);
    w->total+=dt; ++w->count; if(dt>w->max) w->max=dt;
}
bool profiling_get(unsigned id,profiling_result_t *r) {
    if(id>=PROFILING_MAX_STOPWATCHES || !watches[id].name) return false;
    stopwatch_t *w=&watches[id]; *r=(profiling_result_t){w->name,w->count,w->count?(double)w->total/w->count:0,w->max}; return true;
}
void profiling_reset(void) { for(unsigned i=0;i<PROFILING_MAX_STOPWATCHES;++i) { const char *n=watches[i].name; watches[i]=(stopwatch_t){.name=n,.start=time_us_64()}; } }
#endif
