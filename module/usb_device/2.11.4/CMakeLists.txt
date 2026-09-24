# Vendor module: ST USB Device Library (2.11.4). Core plus the MSC, AUDIO, CDC
# and HID class sources. Board independent: the low level configuration header
# (usbd_conf.h) is supplied by the port layer.
add_library(usb_device_inc INTERFACE)
target_include_directories(usb_device_inc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/AUDIO/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Inc)

add_library(usb_device STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbd_ctlreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/usbd_ioreq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbd_msc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbd_msc_bot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbd_msc_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Src/usbd_msc_scsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/AUDIO/Src/usbd_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/CDC/Src/usbd_cdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Src/usbd_hid.c)

target_link_libraries(usb_device PUBLIC usb_device_inc cmsis_core target_${MCU_FAMILY})
# The PCD HAL that usbd_conf.c (port layer) drives.
target_link_libraries(usb_device PRIVATE bsp_drv)
