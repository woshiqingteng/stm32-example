# USB host port: board low-level configuration (HCD + conf + VBUS).
add_library(stm32_usb_host_common_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbh_conf.c)

target_include_directories(stm32_usb_host_common_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_host_common_port PUBLIC bsp_delay bsp_io_expand drv_usb stm32_usb_host)
target_link_libraries(stm32_usb_host_common_port PRIVATE stm32_usb_host_inc)
# The vendor core headers include usbh_conf.h, provided here; expose the board
# conf directory through the module's include interface.
target_include_directories(stm32_usb_host_inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
