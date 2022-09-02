# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/oleksii_khanin/proj/C/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(watch C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

FILE(GLOB Femtox ${CMAKE_CURRENT_LIST_DIR}/femtox/*.c)

add_executable(watch 
        main.c
        ${CMAKE_CURRENT_LIST_DIR}/st7789/st7789.c
        ${CMAKE_CURRENT_LIST_DIR}/st7789/font.c
        ${Femtox}
)

pico_set_program_name(watch "watch")
pico_set_program_version(watch "0.1")

pico_enable_stdio_uart(watch 0)
pico_enable_stdio_usb(watch 1)

# Add the standard library to the build
target_link_libraries(watch pico_stdlib pico_multicore pico_time pico_sync)

# Add any user requested libraries
target_link_libraries(watch
        hardware_spi
        hardware_gpio
        hardware_timer
        hardware_clocks
        )

target_include_directories(watch INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/st7789/
        ${CMAKE_CURRENT_LIST_DIR}/femtox/
)

pico_add_extra_outputs(watch)
