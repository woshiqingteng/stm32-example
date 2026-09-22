# Port layer: USB device/host low level configuration and class interfaces.
# Everything lives in a single static library; an application only pulls the
# objects it references (unused members are dropped by the linker).
set(USB_PORT_SRC)
set(USB_PORT_LINK)

if(MODULE_USB_DEVICE_ENABLE)
    list(APPEND USB_PORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_conf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_desc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_storage_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_audio_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_cdc_if.c)
    list(APPEND USB_PORT_LINK usb_device)
endif()

if(MODULE_USB_HOST_ENABLE)
    list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/usbh_conf.c)
    list(APPEND USB_PORT_LINK usb_host)

    if(MODULE_FATFS_ENABLE)
        list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/usbh_diskio.c)
    endif()
endif()

add_library(usb_port STATIC ${USB_PORT_SRC})
set_target_properties(usb_port PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(usb_port PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MODULE_FATFS_ENABLE)
    target_include_directories(usb_port PRIVATE
        $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

target_link_libraries(usb_port PUBLIC bsp ${USB_PORT_LINK})

# Compile the PCD/HCD HAL sources into the shared HAL target. They are only
# linked into an image when a USB driver references them.
target_sources(stm32f4xx_hal PRIVATE ${HAL_SRC_PCD} ${HAL_SRC_HCD})

# The vendor core headers include usbd_conf.h / usbh_conf.h, which are
# provided here, so expose this directory to those targets.
if(TARGET usb_device)
    target_include_directories(usb_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(TARGET usb_host)
    target_include_directories(usb_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
