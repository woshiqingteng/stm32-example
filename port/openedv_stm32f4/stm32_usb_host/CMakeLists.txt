target_include_directories(stm32_usb_host_inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stm32_usb_host_conf_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbh_conf.c)
target_include_directories(stm32_usb_host_conf_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_host_conf_port PUBLIC bsp_delay bsp_io_expand drv_usb stm32_usb_host)

add_library(stm32_usb_host_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbh_diskio.c)
target_include_directories(stm32_usb_host_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_host_msc_port
    PUBLIC stm32_usb_host_conf_port
    PRIVATE fatfs_core_inc)
