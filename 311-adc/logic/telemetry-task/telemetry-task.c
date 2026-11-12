#include "telemetry-task/telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
static telemetry_state_t state;
static telemetry_task_callback_t callback;
static uint32_t period;
static uint64_t last;
void telemetry_task_init(telemetry_task_callback_t cb) { callback=cb; state=TELEMETRY_STATE_IDLE; }
bool telemetry_task_start(uint32_t ms) { if(ms<10) return false; period=ms; last=systime_us()-(uint64_t)ms*1000; state=TELEMETRY_STATE_RUN; return true; }
void telemetry_task_stop(void) { state=TELEMETRY_STATE_IDLE; }
telemetry_state_t telemetry_task_get_state(void) { return state; }
uint32_t telemetry_task_get_period_ms(void) { return period; }
void telemetry_task_handle(void) {
    switch(state) {
    case TELEMETRY_STATE_IDLE: break;
    case TELEMETRY_STATE_RUN:
        if(systime_period_elapsed(&last,(uint64_t)period*1000)) {
            telemetry_record_t r={systime_us(),platform_voltage_read(),platform_temp_read()}; if(callback) callback(&r);
        } break;
    }
}
