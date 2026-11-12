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

# Compile the production parser/API/tasks against an explicit host platform.
foreach(level RANGE 1 3)
    if(level EQUAL 1)
        set(folder 231-firmware-template)
    elseif(level EQUAL 2)
        set(folder 311-adc)
    else()
        set(folder 321-audio-recorder)
    endif()
    file(GLOB_RECURSE model_logic CONFIGURE_DEPENDS ${REPO}/${folder}/logic/*.c)
    add_executable(model_${level} model_board.c ${model_logic}
        ${REPO}/libs/stdio-text-protocol/src/stdio-text-protocol.c
        ${REPO}/libs/systime/src/systime.c
        ${REPO}/libs/profiling/src/profiling.c)
    target_compile_definitions(model_${level} PRIVATE MODEL_LEVEL=${level} PROFILING_ENABLED=1)
    target_include_directories(model_${level} PRIVATE stubs ${REPO}/${folder} ${REPO}/${folder}/logic)
    foreach(lib stdio-text-protocol systime profiling device logging)
        target_include_directories(model_${level} PRIVATE ${REPO}/libs/${lib}/include)
    endforeach()
    target_compile_options(model_${level} PRIVATE -Wall -Wextra -Werror)
    if(NOT MSVC)
        target_link_libraries(model_${level} PRIVATE m)
    endif()
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
