# Vendor module: ST USB Host Library (3.5.3). Core plus the MSC and HID class
# sources. Board independent: the low level configuration header (usbh_conf.h)
# is supplied by the port layer, which adds its include directory to this target
# (see port/usb_port/CMakeLists.txt).
set(USB_HOST_SRC
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

add_library(usb_host STATIC ${USB_HOST_SRC})
set_target_properties(usb_host PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(usb_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/MSC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Class/HID/Inc)

target_link_libraries(usb_host PUBLIC cmsis_core ${TARGET_LIB})
# The HCD HAL that usbh_conf.c (port layer) drives.
target_link_libraries(usb_host PRIVATE stm32f4xx_hal)
