# Board-independent USB host MSC -> FatFs diskio glue. The board USB host
# directory (usbh_conf.h / usbh_handle.h) is added to the include path.
add_library(stm32_usb_host_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbh_diskio.c)

target_include_directories(stm32_usb_host_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/port/${BSP}/stm32_usb_host)
target_link_libraries(stm32_usb_host_msc_port PUBLIC stm32_usb_host_common_port)
target_link_libraries(stm32_usb_host_msc_port PRIVATE stm32_usb_host_inc fatfs_core_inc)
