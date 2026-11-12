#include "device/device.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/structs/sysinfo.h"
void device_get_info(device_info_t *info)
{
    uint32_t chip=sysinfo_hw->chip_id;
    info->board=PICO_BOARD;
    pico_get_unique_board_id_string(info->serial,sizeof(info->serial));
    info->manufacturer=chip & 0xfff;
    info->part=(chip>>12)&0xffff;
    info->revision=chip>>28;
    info->sdk=PICO_SDK_VERSION_STRING;
}
