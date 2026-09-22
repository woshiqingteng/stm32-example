# Port layer: OS / filesystem / USB adaptation. freertos_port and usb_port are
# still reserved; the FatFs port is added when the core module is enabled.
add_library(port INTERFACE)
target_link_libraries(port INTERFACE bsp)

if(MODULE_FATFS_ENABLE)
    add_subdirectory(fatfs_port)
endif()
