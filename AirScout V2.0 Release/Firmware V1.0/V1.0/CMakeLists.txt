cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


option(USE_VSCODE "Build using vscode" ON)
if(USE_VSCODE)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================


else()
    set(PICO_SDK_FETCH_FROM_GIT ON)
    set(PICO_TOOLCHAIN_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/arm-gnu-toolchain")
endif()




set(PROJECT Airscout)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
set(PICO_EXTRAS_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-extras)
include(pico_extras_import.cmake)
project(${PROJECT} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT})
add_subdirectory(src/)

# Add include files to the build
target_include_directories(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# STDIO over UART 1
pico_enable_stdio_uart(${PROJECT} 0)
pico_enable_stdio_usb(${PROJECT} 1)
target_compile_definitions(${PROJECT} PRIVATE
    PICO_DEFAULT_UART=1
    PICO_DEFAULT_UART_TX_PIN=8
    PICO_DEFAULT_UART_RX_PIN=9
)

# compiler warning
# target_compile_options(${PROJECT} PRIVATE -Wall)

# add_compile_definitions(PICO_MALLOC_PANIC)
# add_compile_definitions(PICO_DEBUG_MALLOC)
# remove_definitions(PICO_MALLOC_PANIC)

# Add the standard library to the build
target_link_libraries(${PROJECT} PUBLIC
    pico_stdlib
)

# Add any user requested internal libraries
target_link_libraries(${PROJECT} PUBLIC
    hardware_spi
    hardware_i2c
    hardware_dma
    hardware_timer
    hardware_clocks
    hardware_pwm
    hardware_sleep
    hardware_adc

    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip
    pico_lwip_iperf
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
)

# add libs
add_subdirectory(lib/)

# additional libraries
target_link_libraries(${PROJECT} PUBLIC
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    BME68x
    minmea
    cjson
    logger
    dhcp
    dns
    sha256
    qr_code
    sps30
)
set_property(TARGET ${PROJECT} APPEND_STRING PROPERTY LINK_FLAGS "-Wl,--print-memory-usage")
pico_add_extra_outputs(${PROJECT})
