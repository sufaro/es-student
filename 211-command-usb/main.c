#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "command.h"
#include "device.h"
#include "led.h"
#include "log.h"
#include "memory.h"

#define LINE_SIZE 64
static char line[LINE_SIZE];
static uint line_length;
static bool line_overflow;
const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

void cmd_enable(void) { led_set(true); LOG_INF("led on\n"); }
void cmd_disable(void) { led_set(false); LOG_INF("led off\n"); }
void cmd_info(void) { device_info(); }
void cmd_version(void) { log_version(); }
void cmd_ping(void) { puts("pong"); }
void cmd_mem_info(void) { mem_info(); }
void cmd_fw_info(void) { fw_info(); }
void cmd_dev_info(void) { dev_info(); }
void cmd_boot_info(void) { boot_info(); }

const struct command_t commands[] = {
    {"enable", cmd_enable},
    {"disable", cmd_disable},
    {"info", cmd_info},
    {"version", cmd_version},
    {"ping", cmd_ping},
    {"mem_info", cmd_mem_info},
    {"fw_info", cmd_fw_info},
    {"dev_info", cmd_dev_info},
    {"boot_info", cmd_boot_info},
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
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    bool stable = gpio_get(BUTTON_PIN);
    bool candidate = stable;
    uint64_t changed_us = time_us_64();
    while (1) {
        bool current = gpio_get(BUTTON_PIN);
        uint64_t now = time_us_64();
        if (current != candidate) {
            candidate = current;
            changed_us = now;
        }
        if (candidate != stable && now - changed_us >= DEBOUNCE_MS * 1000u) {
            stable = candidate;
            if (!stable) {
                led_toggle();
                LOG_INF("led %s\n", led_is_on() ? "on" : "off");
            }
        }
        read_line();
    }
}
