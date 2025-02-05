cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

pico_sdk_init()

project(oevfc1)

set(FAMILY RP2040 CACHE INTERNAL "")
set(FAMILY_MCU RP2040 CACHE INTERNAL "")
set(CMAKE_BUILD_TYPE Debug)

# generate an executable file
add_executable(oevfc1
main.c
rtc_rx8900.c
vfd_if.c
dynamic_control.c
display_control.c
light_sensor.c
)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(oevfc1)

target_link_libraries(oevfc1 pico_stdlib hardware_flash hardware_pwm hardware_i2c hardware_timer hardware_adc pico_multicore)

pico_set_binary_type(oevfc1 copy_to_ram)
  
# compile option
#add_compile_options(-Wall -g3 -O0)