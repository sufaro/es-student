cmake_minimum_required(VERSION 3.15)
project(firmware_host_tests C)
enable_testing()
set(CMAKE_C_STANDARD 11)
set(REPO ${CMAKE_CURRENT_LIST_DIR}/..)
add_executable(core_test core_test.c
    ${REPO}/libs/stdio-text-protocol/src/stdio-text-protocol.c
    ${REPO}/libs/systime/src/systime.c
    ${REPO}/libs/profiling/src/profiling.c
    ${REPO}/231-firmware-template/logic/led-task/led-task.c
    ${REPO}/231-firmware-template/logic/button-task/button-task.c
    ${REPO}/231-firmware-template/logic/pi-task/pi-task.c
    ${REPO}/231-firmware-template/logic/api/api.c
    ${REPO}/231-firmware-template/logic/api/api-callbacks.c)
target_include_directories(core_test PRIVATE stubs ${REPO}/231-firmware-template ${REPO}/231-firmware-template/logic)
foreach(lib stdio-text-protocol systime profiling device logging)
    target_include_directories(core_test PRIVATE ${REPO}/libs/${lib}/include)
endforeach()
add_executable(stream_test stream_test.c
    ${REPO}/321-audio-recorder/logic/stream-task/stream-task.c
    ${REPO}/321-audio-recorder/logic/block-task/block-task.c
    ${REPO}/321-audio-recorder/logic/telemetry-task/telemetry-task.c)
target_include_directories(stream_test PRIVATE ${REPO}/321-audio-recorder ${REPO}/321-audio-recorder/logic ${REPO}/libs/systime/include)
add_executable(buttons_test buttons_test.c ${REPO}/239-two-buttons/logic/button-task/button-task.c)
target_include_directories(buttons_test PRIVATE ${REPO}/239-two-buttons ${REPO}/239-two-buttons/logic ${REPO}/libs/systime/include)
foreach(target core_test stream_test buttons_test)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    add_test(NAME ${target} COMMAND ${target})
endforeach()
