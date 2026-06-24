# dns
add_library(dns INTERFACE)

target_sources(dns INTERFACE
    dnsserver.c
)
target_include_directories(dns INTERFACE
    .
)