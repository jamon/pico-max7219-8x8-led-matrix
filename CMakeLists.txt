cmake_minimum_required(VERSION 3.12)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(test2 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

add_executable(test2 test2.c )
pico_set_program_name(test2 "test2")
pico_set_program_version(test2 "0.1")

#pico_enable_stdio_usb(test2 1)
#pico_enable_stdio_uart(test2 0)

pico_generate_pio_header(test2 ${CMAKE_CURRENT_LIST_DIR}/max7219.pio)

target_link_libraries(test2 PRIVATE pico_stdlib hardware_pio hardware_interp)

pico_add_extra_outputs(test2)

