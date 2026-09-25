# Vendor module: ST USB Host Library (3.5.3). Core plus the MSC and HID class
# sources. Board independent: the low level configuration header (usbh_conf.h)
# is supplied by the port layer.
add_library(usb_host_inc INTERFACE)
target_include_directories(usb_host_inc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Inc)

add_library(usb_host STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbh_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbh_ctlreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbh_ioreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbh_pipes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbh_msc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbh_msc_bot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbh_msc_scsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Src/usbh_hid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Src/usbh_hid_keybd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Src/usbh_hid_mouse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Src/usbh_hid_parser.c)

target_link_libraries(usb_host PUBLIC usb_host_inc cmsis_core soc_${MCU_FAMILY})
# The HCD HAL that usbh_conf.c (port layer) drives.
target_link_libraries(usb_host PRIVATE bsp_drv)
