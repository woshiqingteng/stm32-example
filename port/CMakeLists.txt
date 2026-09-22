# Port layer: OS / filesystem / USB adaptation. Reserved for freertos_port,
# fatfs_port and usb_port; empty for now.
add_library(port INTERFACE)
target_link_libraries(port INTERFACE bsp)
