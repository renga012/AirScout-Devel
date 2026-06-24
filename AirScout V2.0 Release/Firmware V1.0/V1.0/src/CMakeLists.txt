add_subdirectory(driver)
add_subdirectory(modes)
add_subdirectory(gui)
target_sources(${PROJECT} PRIVATE 
    hw_config.cpp
    settings.cpp
    utils.cpp
    wifi.cpp
    main.cpp
    ntp.cpp
)