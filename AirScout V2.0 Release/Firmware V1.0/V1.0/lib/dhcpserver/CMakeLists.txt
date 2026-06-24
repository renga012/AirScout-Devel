# dhcp
add_library(dhcp INTERFACE)

target_sources(dhcp INTERFACE
    dhcpserver.c
)
target_include_directories(dhcp INTERFACE
    .
)