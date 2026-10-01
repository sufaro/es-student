#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "command.h"
#include "device.h"
#include "led.h"
#include "log.h"
#include "memory.h"
#include "clock.h"
#include "profiling.h"

#define LINE_SIZE 64
static char line[LINE_SIZE];
static uint line_length;
static bool line_overflow;
const uint BLINK_HALF_PERIOD_MS = 500;
// About 600 software floating-point cycles per term: 1e6 * 600 / 125 MHz = 4.8 s.
const uint CALC_PI_TERMS = 1000000;
static volatile double pi_result;

double calc_pi(uint terms)
{
    double sum = 0.0;
    for (uint k = 0; k < terms; ++k) {
        double term = 1.0 / (2.0 * k + 1.0);
        sum += (k & 1u) ? -term : term;
    }
    return 4.0 * sum;
}
void cmd_calc_pi(void)
{
    uint64_t start = time_us_64();
    pi_result = calc_pi(CALC_PI_TERMS);
    uint64_t spent = time_us_64() - start;
    printf("pi: %.8f\n", pi_result);
    printf("time: %llu ms\n", (unsigned long long)(spent / 1000));
}
void cmd_clk_info(void) { clk_info(); }
void cmd_uptime(void) { uptime(); }
void cmd_clk_sys_low(void) { clk_sys_low(); }
void cmd_clk_sys_default(void) { clk_sys_default(); }
void cmd_main_time_exec(void) {
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(), (unsigned)profiling_max_us());
}
void cmd_main_time_reset(void) { profiling_reset_max(); puts("max reset"); }

void cmd_info(void) { device_info(); }
void cmd_version(void) { log_version(); }
void cmd_ping(void) { puts("pong"); }
void cmd_mem_info(void) { mem_info(); }
void cmd_fw_info(void) { fw_info(); }
void cmd_dev_info(void) { dev_info(); }
void cmd_boot_info(void) { boot_info(); }

const struct command_t commands[] = {
    {"info", cmd_info},
    {"version", cmd_version},
    {"ping", cmd_ping},
    {"mem_info", cmd_mem_info},
    {"fw_info", cmd_fw_info},
    {"dev_info", cmd_dev_info},
    {"boot_info", cmd_boot_info},
    {"clk_info", cmd_clk_info},
    {"uptime", cmd_uptime},
    {"calc_pi", cmd_calc_pi},
    {"main_time_exec", cmd_main_time_exec},
    {"main_time_reset", cmd_main_time_reset},
    {"clk_sys_low", cmd_clk_sys_low},
    {"clk_sys_default", cmd_clk_sys_default},
};
const uint command_count = sizeof(commands) / sizeof(commands[0]);

void handle_command(const char *command)
{
    for (uint i = 0; i < command_count; ++i) {
        if (strcmp(command, commands[i].name) == 0) {
            if (commands[i].handler != NULL) {
                commands[i].handler();
            }
            return;
        }
    }
    LOG_ERR("unknown command: %s\n", command);
}

static void read_line(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT) return;
    if (symbol == '\r' || symbol == '\n') {
        putchar('\n');
        line[line_length] = '\0';
        if (line_overflow) {
            LOG_ERR("command too long\n");
        } else if (line_length > 0) {
            handle_command(line);
        }
        line_length = 0;
        line_overflow = false;
        return;
    }
    if (symbol == '\b' || symbol == 127) {
        if (!line_overflow && line_length > 0) {
            --line_length;
            printf("\b \b");
        }
        return;
    }
    if (symbol < 32 || symbol > 126) return;
    putchar(symbol);
    if (line_length + 1 < sizeof(line) && !line_overflow) {
        line[line_length++] = (char)symbol;
    } else {
        line_overflow = true;
    }
}

int main(void)
{
    stdio_init_all();
    led_init();
    uint64_t previous = time_us_64();
    profiling_init();
    while (1) {
        profiling_iteration();
        uint64_t now = time_us_64();
        if (now - previous >= BLINK_HALF_PERIOD_MS * 1000u) {
            previous = now;
            led_toggle();
        }
        read_line();
    }
}
