#include "clock.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/regs/clocks.h"
#include "log.h"

static const uint CLK_SYS_LOW_KHZ = 62500;

void clk_info(void)
{
    static const struct {
        const char *name;
        clock_handle_t clock;
        uint source;
    } signals[] = {
        {"clk_ref", clk_ref, CLOCKS_FC0_SRC_VALUE_CLK_REF},
        {"clk_sys", clk_sys, CLOCKS_FC0_SRC_VALUE_CLK_SYS},
        {"clk_peri", clk_peri, CLOCKS_FC0_SRC_VALUE_CLK_PERI},
        {"clk_usb", clk_usb, CLOCKS_FC0_SRC_VALUE_CLK_USB},
        {"clk_adc", clk_adc, CLOCKS_FC0_SRC_VALUE_CLK_ADC},
    };
    puts("clock configured_khz measured_khz");
    for (uint i = 0; i < count_of(signals); ++i) {
        printf("%s %u %u\n", signals[i].name,
               (unsigned)(clock_get_hz(signals[i].clock) / 1000),
               (unsigned)frequency_count_khz(signals[i].source));
    }
    printf("rosc - %u\n", (unsigned)frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}

void uptime(void)
{
    printf("uptime: %llu ms\n", (unsigned long long)(time_us_64() / 1000));
}

// set_sys_clock_pll switches clk_peri to PLL_USB (48 MHz) unless configured otherwise.
static void clk_sys_set(uint khz)
{
    if (!set_sys_clock_khz(khz, false)) {
        LOG_ERR("clk_sys not set to %u kHz\n", khz);
        return;
    }
    printf("clk_sys set to %u kHz\n", khz);
}
void clk_sys_low(void) { clk_sys_set(CLK_SYS_LOW_KHZ); }
void clk_sys_default(void) { clk_sys_set(SYS_CLK_KHZ); }
