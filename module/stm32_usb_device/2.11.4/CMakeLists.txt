add_library(stm32_usb_device_inc INTERFACE)
target_include_directories(stm32_usb_device_inc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/AUDIO/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Inc)

add_library(stm32_usb_device STATIC EXCLUDE_FROM_ALL
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

target_link_libraries(stm32_usb_device PUBLIC stm32_usb_device_inc cmsis_core soc_${MCU_FAMILY})
target_link_libraries(stm32_usb_device PRIVATE drv_usb)
