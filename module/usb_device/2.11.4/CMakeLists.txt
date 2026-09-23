# Vendor module: ST USB Device Library (2.11.4). Core plus the MSC, AUDIO, CDC
# and HID class sources. Board independent: the low level configuration header
# (usbd_conf.h) is supplied by the port layer, which adds its include directory
# to this target (see port/usb_port/CMakeLists.txt).
set(USB_DEVICE_SRC
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

add_library(usb_device STATIC ${USB_DEVICE_SRC})
set_target_properties(usb_device PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(usb_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/AUDIO/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Inc)

target_link_libraries(usb_device PUBLIC cmsis_core ${TARGET_LIB})
# The PCD HAL that usbd_conf.c (port layer) drives.
target_link_libraries(usb_device PRIVATE stm32f4xx_hal)
