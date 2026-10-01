#include "memory.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"
#include "command.h"
#include "device.h"
#include "led.h"
#include "log.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;
int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-12s 0x%08" PRIxPTR " 0x%08" PRIxPTR " %" PRIuPTR "\n",
           name, start, end, end - start);
}

void mem_info(void)
{
    uintptr_t data_size = (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__;
    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_BASE + 264u * 1024u);
    row("rom", ROM_BASE, ROM_BASE + 16u * 1024u);
    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);
    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + data_size);
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);
    puts("total");
    printf("flash image %" PRIuPTR "\n",
           (uintptr_t)&__flash_binary_end - (uintptr_t)&__flash_binary_start);
    printf("flash free %" PRIuPTR "\n",
           XIP_BASE + PICO_FLASH_SIZE_BYTES - (uintptr_t)&__flash_binary_end);
    printf("ram used %" PRIuPTR "\n",
           (uintptr_t)&__bss_end__ - (uintptr_t)&__data_start__);
    printf("ram free %" PRIuPTR "\n",
           (uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__);
}

void fw_info(void)
{
    const uint16_t *main_code = (const uint16_t *)((uintptr_t)main & ~1u);
    const uint16_t *fw_code = (const uint16_t *)((uintptr_t)fw_info & ~1u);
    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));
    if (heap_variable == NULL) {
        LOG_ERR("malloc failed\n");
        return;
    }
    *heap_variable = 1951;
    ++data_variable;
    ++bss_variable;
    printf("main 0x%08" PRIxPTR " 0x%04x\n", (uintptr_t)main, *main_code);
    printf("fw_info 0x%08" PRIxPTR " 0x%04x\n", (uintptr_t)fw_info, *fw_code);
    printf("commands 0x%08" PRIxPTR "\n", (uintptr_t)commands);
    for (uint i = 0; i < command_count; ++i) {
        printf("- %s 0x%08" PRIxPTR " 0x%08" PRIxPTR "\n",
               commands[i].name, (uintptr_t)commands[i].name,
               (uintptr_t)commands[i].handler);
    }
    printf("DEVICE_PROJECT 0x%08" PRIxPTR " %s\n", (uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("DEVICE_BOARD 0x%08" PRIxPTR " %s\n", (uintptr_t)DEVICE_BOARD, DEVICE_BOARD);
    printf("data_variable 0x%08" PRIxPTR " %" PRIu32 "\n", (uintptr_t)&data_variable, data_variable);
    printf("bss_variable 0x%08" PRIxPTR " %" PRIu32 "\n", (uintptr_t)&bss_variable, bss_variable);
    printf("stack_variable 0x%08" PRIxPTR " %" PRIu32 "\n", (uintptr_t)&stack_variable, stack_variable);
    printf("heap_variable 0x%08" PRIxPTR " %" PRIu32 "\n", (uintptr_t)heap_variable, *heap_variable);
    free(heap_variable);
}

void boot_info(void)
{
    const uint32_t *vectors = (const uint32_t *)0x10000100;
    volatile uint32_t *gpio_in = (volatile uint32_t *)(SIO_BASE + SIO_GPIO_IN_OFFSET);
    uint32_t level = (*gpio_in >> led_pin()) & 1u;
    printf("vector table 0x10000100\n");
    printf("stack top 0x%08" PRIx32 "\n", vectors[0]);
    printf("reset 0x%08" PRIx32 "\n", vectors[1]);
    printf("reset (even) 0x%08" PRIx32 "\n", vectors[1] & ~1u);
    printf("gpio in 0x%08" PRIxPTR "\n", (uintptr_t)gpio_in);
    printf("led bit %" PRIu32 "\n", level);
    printf("gpio_get %u\n", gpio_get(led_pin()));
}
