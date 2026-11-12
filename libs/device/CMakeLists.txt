add_library(device STATIC src/device.c)
target_include_directories(device PUBLIC include)
target_compile_options(device PRIVATE -Wall -Wextra -Werror)
target_link_libraries(device PRIVATE pico_stdlib pico_unique_id)
