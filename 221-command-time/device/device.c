#include "device.h"
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/structs/sysinfo.h"
#include "hardware/regs/sysinfo.h"

struct info_t device_card = {
    .version = 0x00010000,
    .name = "es-student",
    .revision = 1,
};

void device_info(void)
{
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));
    uint32_t id = sysinfo_hw->chip_id;
    printf("project: %s\nrepo: %s\nboard: %s\nserial: %s\npico-sdk: %s\n",
           DEVICE_PROJECT, DEVICE_REPO, DEVICE_BOARD, board_id, PICO_SDK_VERSION_STRING);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)((id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB),
           (unsigned)((id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB),
           (unsigned)((id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB));
}

void dev_info(void)
{
    puts("struct address size offset value");
    printf("device_card 0x%08" PRIxPTR " %u\n", (uintptr_t)&device_card, (unsigned)sizeof(device_card));
    printf("- version 0x%08" PRIxPTR " %u %u 0x%08" PRIx32 "\n",
           (uintptr_t)&device_card.version, (unsigned)sizeof(device_card.version),
           (unsigned)offsetof(struct info_t, version), device_card.version);
    printf("- name 0x%08" PRIxPTR " %u %u %s\n",
           (uintptr_t)&device_card.name, (unsigned)sizeof(device_card.name),
           (unsigned)offsetof(struct info_t, name), device_card.name);
    printf("- revision 0x%08" PRIxPTR " %u %u %u\n",
           (uintptr_t)&device_card.revision, (unsigned)sizeof(device_card.revision),
           (unsigned)offsetof(struct info_t, revision), device_card.revision);
    unsigned fields = sizeof(device_card.version) + sizeof(device_card.name) + sizeof(device_card.revision);
    printf("fields %u, sizeof %u, padding %u\n", fields,
           (unsigned)sizeof(device_card), (unsigned)sizeof(device_card) - fields);
}
