
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_COPY_TO_RAM,1)

# Set flags and directory variables
set(FREERTOS_CFG_DIRECTORY "${CMAKE_SOURCE_DIR}/Config")
set(FREERTOS_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel")

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project (ndrnkc C CXX ASM)

pico_sdk_init ()


# Add FreeRTOS as a library
add_library(FreeRTOS STATIC
    ${FREERTOS_SRC_DIRECTORY}/event_groups.c
    ${FREERTOS_SRC_DIRECTORY}/list.c
    ${FREERTOS_SRC_DIRECTORY}/queue.c
    ${FREERTOS_SRC_DIRECTORY}/stream_buffer.c
    ${FREERTOS_SRC_DIRECTORY}/tasks.c
    ${FREERTOS_SRC_DIRECTORY}/timers.c
    ${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_3.c
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0/port.c
)

# Build FreeRTOS
target_include_directories(FreeRTOS PUBLIC
    ${FREERTOS_CFG_DIRECTORY}/
    ${FREERTOS_SRC_DIRECTORY}/include
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0
#    ${FREERTOS_SRC_DIRECTORY}/portable/ThirdParty/GCC/RP2040
)


add_executable(ndrnkc)

pico_generate_pio_header(ndrnkc ${CMAKE_CURRENT_LIST_DIR}/par_bus.pio)
pico_generate_pio_header(ndrnkc ${CMAKE_CURRENT_LIST_DIR}/gdp.pio)
pico_generate_pio_header(ndrnkc ${CMAKE_CURRENT_LIST_DIR}/ps2key.pio)

target_sources(ndrnkc PRIVATE ndrnkc.c parport.S cas_io.S key_io.S gdp_io.S
  xmodem_pico.c gdp.c gdp_char.c gdp_line.c
  helper.c
  cas.c
  ps2key.c
  stdio_dev.c
  par_bus.c)

target_link_libraries(
  ndrnkc PRIVATE
  pico_stdlib
  FreeRTOS
  hardware_pio
  hardware_dma
  hardware_irq
  hardware_interp
  pico_multicore)

# enable usb output, disable uart output
pico_enable_stdio_usb(ndrnkc 1)
pico_enable_stdio_uart(ndrnkc 0)

pico_add_extra_outputs(ndrnkc)
