#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "tusb.h"
#include <string.h>
#define TONE_PIN 20
static void recorder_init(void);
static bool indicator_active;
static bool led_requested;
static bool hung;
static bool watchdog_active;
static bool reset_watchdog;
static uint32_t restarts;
#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096
#define CHARGE_PIN 22
#define LED_PIN 25
#define BUTTON_PIN 15
void platform_init(void) {
    stdio_init_all(); recorder_init();
    adc_init(); adc_gpio_init(ADC_PIN); adc_set_temp_sensor_enabled(true);
    gpio_init(CHARGE_PIN); gpio_set_dir(CHARGE_PIN,GPIO_OUT); gpio_put(CHARGE_PIN,0); gpio_init(LED_PIN); gpio_set_dir(LED_PIN,GPIO_OUT);
    gpio_init(BUTTON_PIN); gpio_set_dir(BUTTON_PIN,GPIO_IN); gpio_pull_up(BUTTON_PIN);
}
void platform_led_set(bool on) { led_requested=on; if(!indicator_active) gpio_put(LED_PIN,on); }
bool platform_button_read(void) { return !gpio_get(BUTTON_PIN); }



float platform_voltage_read(void) { adc_select_input(ADC_CHANNEL); return adc_read()*ADC_VREF_V/ADC_LEVELS; }
float platform_temp_read(void) { adc_select_input(ADC_TEMP_CHANNEL); float voltage=adc_read()*ADC_VREF_V/ADC_LEVELS; return 27.0f-(voltage-0.706f)/0.001721f; }
void platform_charge_set(bool on) { gpio_put(CHARGE_PIN,on); }




typedef enum { ADC_OWNER_IDLE, ADC_OWNER_CAPTURE, ADC_OWNER_BLOCK, ADC_OWNER_STREAM } adc_owner_t;
static volatile adc_owner_t adc_owner;
static struct repeating_timer indicator_timer,capture_timer;
static uint32_t indicator_toggles;
static bool indicator_on;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static volatile uint32_t capture_index;
static uint32_t capture_count,capture_period,capture_work;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static uint32_t block_count,block_started;
static volatile uint32_t block_elapsed;
static volatile bool block_ready;
static int channels[2];
static uint8_t stream_buffers[2][PLATFORM_STREAM_BYTES] __attribute__((aligned(4)));
static volatile bool stream_ready[2];
static volatile uint16_t stream_sequence[2];
static uint16_t next_sequence;
static unsigned stream_bits;
static volatile uint32_t stream_overrun;
_Static_assert(sizeof(capture_samples)+sizeof(capture_times)+sizeof(block_samples)+sizeof(stream_buffers)<=32768,"capture buffer budget");
bool platform_adc_busy(void) { return adc_owner!=ADC_OWNER_IDLE; }
static bool indicator_irq(struct repeating_timer *timer) {
    (void)timer; indicator_on=!indicator_on; gpio_put(LED_PIN,indicator_on); ++indicator_toggles; return true;
}
uint32_t platform_timer_blink_stop(void) {
    if(indicator_active) cancel_repeating_timer(&indicator_timer);
    indicator_active=false; gpio_put(LED_PIN,led_requested); return indicator_toggles;
}
bool platform_timer_blink_start(uint32_t period_ms) {
    if(period_ms<10 || period_ms>60000) return false;
    platform_timer_blink_stop(); indicator_toggles=0; indicator_on=false;
    indicator_active=add_repeating_timer_ms(-(int32_t)period_ms,indicator_irq,NULL,&indicator_timer);
    return indicator_active;
}
static bool capture_irq(struct repeating_timer *timer) {
    (void)timer; uint32_t start=time_us_32(),i=capture_index;
    capture_times[i]=start; capture_samples[i]=adc_read(); capture_work+=time_us_32()-start;
    __dmb(); capture_index=i+1;
    if(capture_index==capture_count) { adc_owner=ADC_OWNER_IDLE; return false; } return true;
}
bool platform_capture_start(uint32_t count,uint32_t period_us) {
    if(platform_adc_busy() || !count || count>PLATFORM_CAPTURE_MAX || period_us<10 || period_us>INT32_MAX) return false;
    capture_count=count; capture_period=period_us; capture_index=0; capture_work=0;
    adc_owner=ADC_OWNER_CAPTURE; adc_select_input(ADC_CHANNEL);
    if(!add_repeating_timer_us(-(int64_t)period_us,capture_irq,NULL,&capture_timer)) { adc_owner=ADC_OWNER_IDLE; return false; }
    return true;
}
bool platform_capture_done(void) { return capture_count && capture_index==capture_count; }
void platform_capture_get(platform_capture_t *r) {
    uint32_t lo=UINT32_MAX,hi=0;
    for(uint32_t i=1;i<capture_count;++i) { uint32_t dt=capture_times[i]-capture_times[i-1]; if(dt<lo) lo=dt; if(dt>hi) hi=dt; }
    uint32_t span=capture_count>1?capture_times[capture_count-1]-capture_times[0]:capture_period;
    *r=(platform_capture_t){capture_samples,capture_count,capture_period,capture_count>1?lo:0,hi,span?100.0f*capture_work/span:0};
}
static void setup_adc(bool shift) {
    adc_run(false); adc_fifo_drain(); adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true,true,1,false,shift);
    // 48 MHz / 44100 cycles per sample; the SDK writes the divider's 8-bit fraction.
    adc_set_clkdiv((float)clock_get_hz(clk_adc)/44100.0f-1.0f);
}
static void recorder_dma_irq(void) {
    uint32_t pending=dma_hw->ints0;
    for(unsigned i=0;i<2;++i) if(pending & (1u<<channels[i])) {
        dma_hw->ints0=1u<<channels[i];
        if(adc_owner==ADC_OWNER_BLOCK && i==0) {
            adc_run(false); block_elapsed=time_us_32()-block_started; __dmb(); block_ready=true; adc_owner=ADC_OWNER_IDLE;
        } else if(adc_owner==ADC_OWNER_STREAM) {
            unsigned other=i^1u;
            // The chain has already started writing the other buffer. Its old data expired.
            if(stream_ready[other]) { stream_ready[other]=false; ++stream_overrun; }
            stream_sequence[i]=next_sequence++; __dmb(); stream_ready[i]=true;
            dma_channel_set_write_addr(channels[i],stream_buffers[i],false);
            dma_channel_set_trans_count(channels[i],stream_bits==16?512:1024,false);
        }
    }
}
static void recorder_init(void) {
    reset_watchdog=watchdog_enable_caused_reboot();
    restarts=reset_watchdog && watchdog_hw->scratch[0]==0x45535243?watchdog_hw->scratch[1]+1:0;
    watchdog_hw->scratch[0]=0x45535243; watchdog_hw->scratch[1]=restarts;
    channels[0]=dma_claim_unused_channel(true); channels[1]=dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0,recorder_dma_irq); irq_set_enabled(DMA_IRQ_0,true);
    gpio_set_function(TONE_PIN,GPIO_FUNC_PWM); pwm_set_gpio_level(TONE_PIN,0);
}
bool platform_block_start(uint32_t count) {
    if(platform_adc_busy() || !count || count>PLATFORM_BLOCK_MAX) return false;
    adc_owner=ADC_OWNER_BLOCK; block_count=count; block_ready=false; setup_adc(false);
    dma_channel_config config=dma_channel_get_default_config(channels[0]);
    channel_config_set_transfer_data_size(&config,DMA_SIZE_16); channel_config_set_read_increment(&config,false);
    channel_config_set_write_increment(&config,true); channel_config_set_dreq(&config,DREQ_ADC);
    dma_hw->ints0=1u<<channels[0]; dma_channel_set_irq0_enabled(channels[0],true);
    dma_channel_configure(channels[0],&config,block_samples,&adc_hw->fifo,count,false);
    block_started=time_us_32(); dma_start_channel_mask(1u<<channels[0]); adc_run(true); return true;
}
bool platform_block_done(void) { return block_ready; }
void platform_block_get(platform_block_t *r) { *r=(platform_block_t){block_samples,block_count,block_elapsed}; }
bool platform_stream_start(unsigned bits) {
    if(platform_adc_busy() || (bits!=8 && bits!=16)) return false;
    stream_bits=bits; stream_overrun=0; next_sequence=0; stream_ready[0]=stream_ready[1]=false;
    adc_owner=ADC_OWNER_STREAM; setup_adc(bits==8);
    for(unsigned i=0;i<2;++i) {
        dma_channel_config config=dma_channel_get_default_config(channels[i]);
        channel_config_set_transfer_data_size(&config,bits==16?DMA_SIZE_16:DMA_SIZE_8);
        channel_config_set_read_increment(&config,false); channel_config_set_write_increment(&config,true);
        channel_config_set_dreq(&config,DREQ_ADC); channel_config_set_chain_to(&config,channels[i^1u]);
        dma_hw->ints0=1u<<channels[i]; dma_channel_set_irq0_enabled(channels[i],true);
        dma_channel_configure(channels[i],&config,stream_buffers[i],&adc_hw->fifo,bits==16?512:1024,false);
    }
    dma_start_channel_mask(1u<<channels[0]); adc_run(true); return true;
}
void platform_stream_stop(void) {
    if(adc_owner!=ADC_OWNER_STREAM) return;
    adc_run(false);
    uint32_t saved=save_and_disable_interrupts(); adc_owner=ADC_OWNER_IDLE;
    // Break BOTH chain links before aborting: abort completion must not restart its partner.
    for(unsigned i=0;i<2;++i) {
        dma_channel_set_irq0_enabled(channels[i],false);
        dma_channel_config config=dma_get_channel_config(channels[i]);
        channel_config_set_chain_to(&config,channels[i]); dma_channel_set_config(channels[i],&config,false);
    }
    for(unsigned i=0;i<2;++i) { dma_channel_abort(channels[i]); dma_hw->ints0=1u<<channels[i]; stream_ready[i]=false; }
    adc_fifo_drain(); restore_interrupts(saved);
}
bool platform_stream_take(uint8_t *samples,uint16_t *sequence) {
    uint32_t saved=save_and_disable_interrupts();
    for(unsigned i=0;i<2;++i) if(stream_ready[i]) {
        // IRQ delivery may be pending while DMA has already chained into this buffer.
        if(dma_channel_is_busy(channels[i])) { stream_ready[i]=false; ++stream_overrun; continue; }
        memcpy(samples,stream_buffers[i],PLATFORM_STREAM_BYTES); *sequence=stream_sequence[i]; stream_ready[i]=false;
        bool valid=!dma_channel_is_busy(channels[i]); if(!valid) ++stream_overrun;
        restore_interrupts(saved); return valid;
    }
    restore_interrupts(saved); return false;
}
uint32_t platform_stream_overruns(void) { return stream_overrun; }
bool platform_frame_write(const void *data,size_t size) {
    // Enqueue a whole frame atomically. A full USB queue defers it to a later superloop iteration.
    uint32_t saved=save_and_disable_interrupts();
    bool accepted=tud_cdc_connected() && tud_cdc_write_available()>=size;
    if(accepted) { stdio_put_string((const char *)data,(int)size,false,false); }
    restore_interrupts(saved); return accepted;
}
void platform_generate_tone(uint32_t hz) {
    unsigned slice=pwm_gpio_to_slice_num(TONE_PIN);
    if(!hz) { pwm_set_enabled(slice,false); gpio_set_function(TONE_PIN,GPIO_FUNC_SIO); gpio_set_dir(TONE_PIN,GPIO_OUT); gpio_put(TONE_PIN,0); return; }
    // Fixed divider; TOP and compare are double-buffered and latch together at wrap.
    gpio_set_function(TONE_PIN,GPIO_FUNC_PWM); float divider=128.0f; uint32_t top=(uint32_t)((float)clock_get_hz(clk_sys)/(divider*hz)+0.5f)-1;
    pwm_set_clkdiv(slice,divider); pwm_set_wrap(slice,(uint16_t)top); pwm_set_gpio_level(TONE_PIN,(top+1)/2); pwm_set_enabled(slice,true);
}
void platform_watchdog_enable(uint32_t ms) { watchdog_enable(ms,true); watchdog_active=true; }
void platform_watchdog_update(void) { if(watchdog_active) watchdog_update(); }
bool platform_reset_is_watchdog(void) { return reset_watchdog; }
uint32_t platform_reset_count(void) { return restarts; }
void platform_hang(void) { hung=true; }
bool platform_is_hung(void) { return hung; }
