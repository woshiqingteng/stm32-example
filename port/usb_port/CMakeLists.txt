# Port layer: USB device/host low level configuration and class interfaces.
# Like fatfs_port, this library only takes the vendor headers (no link edge) so
# the consumer keeps usb_device/usb_host *before* usb_port in the static archive
# order (the LL callbacks here implement symbols referenced by the vendor core).
set(USB_PORT_SRC)

if(MODULE_USB_DEVICE_ENABLE)
    list(APPEND USB_PORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_conf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_desc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_storage_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_audio_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_cdc_if.c)
endif()

if(MODULE_USB_HOST_ENABLE)
    list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/usbh_conf.c)

    if(MODULE_FATFS_ENABLE)
        list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/usbh_diskio.c)
    endif()
endif()

add_library(usb_port STATIC ${USB_PORT_SRC})

target_include_directories(usb_port PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(usb_port PUBLIC bsp)

# Vendor USB core headers only; the USB stack libraries are linked by the
# consumer (lib_usb_device / lib_usb_host) to preserve archive order.
if(TARGET usb_device)
    target_link_libraries(usb_port PRIVATE usb_device_inc)
    # The vendor core headers include usbd_conf.h, provided here.
    target_include_directories(usb_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
if(TARGET usb_host)
    target_link_libraries(usb_port PRIVATE usb_host_inc)
    target_include_directories(usb_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# usbh_diskio.c includes "ff.h".
if(MODULE_FATFS_ENABLE)
    target_link_libraries(usb_port PRIVATE fatfs_core_inc)
endif()
