#pragma once
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "telemetry-task/telemetry-task.h"
void api_print_telemetry(const telemetry_record_t *record);
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"
void api_print_block(const block_result_t *block);
bool api_send_frame(const stream_frame_t *frame);
void api_handle(const command_t *command);
