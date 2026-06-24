# sd Card
add_subdirectory(fatfs/src build)

# bme68x
add_library(BME68x INTERFACE)

target_sources(BME68x INTERFACE
    BME68x_SensorAPI/bme68x.c
)

target_include_directories(BME68x INTERFACE
    BME68x_SensorAPI
)

# minmea
add_library(minmea INTERFACE)
target_sources(minmea INTERFACE
    minmea/minmea.c
)

# timegm not available in pico sdk
target_compile_definitions(minmea INTERFACE timegm=mktime)

target_include_directories(minmea INTERFACE
    minmea
)

# cjson
add_library(cjson INTERFACE)
target_sources(cjson INTERFACE
    cJSON/cJSON.c
)
target_include_directories(cjson INTERFACE
    cJSON/
)

# logging
add_library(logger INTERFACE)
target_sources(logger INTERFACE
    microlog/src/ulog.c
)
target_include_directories(logger INTERFACE
    microlog/include
)

target_compile_definitions(logger INTERFACE
    ULOG_BUILD_COLOR=1 #Compile color code paths
    # ULOG_BUILD_PREFIX_SIZE=0 #Prefix buffer logic
    # ULOG_BUILD_EXTRA_OUTPUTS=0 #Extra output backends
    ULOG_BUILD_SOURCE_LOCATION=0 #File:line output
    # ULOG_BUILD_LEVEL_SHORT=0 #Print levels with short names, e.g. 'E'
    ULOG_BUILD_TIME=1 #Timestamp support
    # ULOG_BUILD_TOPICS_MODE=ULOG_BUILD_TOPICS_MODE_OFF #Topic allocation mode
    # ULOG_BUILD_TOPICS_STATIC_NUM=0 #Number of static topics (0 = disabled)
    # ULOG_BUILD_DYNAMIC_CONFIG=0 #Runtime toggles
    # ULOG_BUILD_WARN_NOT_ENABLED=1 #Warning stubs
    # ULOG_BUILD_CONFIG_HEADER_ENABLED=0 #Use external configuration header
    # ULOG_BUILD_CONFIG_HEADER_NAME="ulog_config.h" #Configuration header name
    # ULOG_BUILD_DISABLED=0 # Disable microlog completely
)
# dhcp
add_subdirectory(dhcpserver)

# dns 
add_subdirectory(dnsserver)

# sha256
add_library(sha256 INTERFACE)
target_sources(sha256 INTERFACE
    crypto-algorithms/sha256.c
)
target_include_directories(sha256 INTERFACE
    crypto-algorithms
)

# QR-Code
add_library(qr_code INTERFACE)
target_sources(qr_code INTERFACE
    QRCode/src/qrcode.c
)
target_include_directories(qr_code INTERFACE
    QRCode/src/
)

# sps30 lib
add_library(sps30 INTERFACE)
target_sources(sps30 INTERFACE
    embedded-i2c-sps30/sps30_i2c.c
    embedded-i2c-sps30/sensirion_i2c.c
    embedded-i2c-sps30/sensirion_common.c
)
target_include_directories(sps30 INTERFACE
    embedded-i2c-sps30
)
