#pragma once
#include <stdbool.h>
void platform_init(void);
float platform_voltage_read(void);
float platform_temp_read(void);
void platform_charge_set(bool on);
void platform_led_set(bool on);
bool platform_button_read(void);


#include <stddef.h>
#include <stdint.h>
#define PLATFORM_CAPTURE_MAX 2000
#define PLATFORM_BLOCK_MAX 8192
#define PLATFORM_STREAM_BYTES 1024
typedef struct { const uint16_t *samples; uint32_t count,period_us,min_interval_us,max_interval_us; float handler_percent; } platform_capture_t;
typedef struct { const uint16_t *samples; uint32_t count,elapsed_us; } platform_block_t;
bool platform_adc_busy(void);
bool platform_timer_blink_start(uint32_t period_ms);
uint32_t platform_timer_blink_stop(void);
bool platform_capture_start(uint32_t count,uint32_t period_us);
bool platform_capture_done(void);
void platform_capture_get(platform_capture_t *capture);
bool platform_block_start(uint32_t count);
bool platform_block_done(void);
void platform_block_get(platform_block_t *block);
bool platform_stream_start(unsigned bits);
void platform_stream_stop(void);
bool platform_stream_take(uint8_t *samples,uint16_t *sequence);
uint32_t platform_stream_overruns(void);
bool platform_frame_write(const void *data,size_t size);
void platform_generate_tone(uint32_t hz);
void platform_watchdog_enable(uint32_t timeout_ms);
void platform_watchdog_update(void);
bool platform_reset_is_watchdog(void);
uint32_t platform_reset_count(void);
void platform_hang(void);
bool platform_is_hung(void);
