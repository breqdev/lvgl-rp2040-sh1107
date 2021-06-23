cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(lvgl_rp2040_sh1107)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_subdirectory(lvgl)


add_executable(
    lvgl_rp2040_sh1107
    main.c
)

target_link_libraries(lvgl_rp2040_sh1107 pico_stdlib hardware_i2c lvgl)

pico_add_extra_outputs(lvgl_rp2040_sh1107)

pico_enable_stdio_usb(lvgl_rp2040_sh1107 1)
pico_enable_stdio_uart(lvgl_rp2040_sh1107 0)
