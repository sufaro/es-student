#include "pi-task/pi-task.h"
#include "systime/systime.h"
#define PI_TERMS_PER_CALL 32
static pi_state_t state;
static uint32_t terms, index;
static double sum;
static uint64_t started, elapsed;
void pi_task_init(void) { state=PI_STATE_IDLE; }
bool pi_task_start(uint32_t count) {
    if(!count || state==PI_STATE_RUN) return false;
    terms=count; index=0; sum=0; elapsed=0; started=systime_ms(); state=PI_STATE_RUN; return true;
}
void pi_task_handle(void) {
    if(state!=PI_STATE_RUN) return;
    for(unsigned n=0;n<PI_TERMS_PER_CALL && index<terms;++n,++index)
        sum+=(index&1?-1.0:1.0)/(2.0*(double)index+1.0);
    if(index==terms) { elapsed=systime_ms()-started; state=PI_STATE_DONE; }
}
void pi_task_get_result(pi_result_t *r) { *r=(pi_result_t){state,4.0*sum,elapsed}; }
