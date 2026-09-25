# Port layer: vendor-stack -> hardware adaptation.
# Layout groups by nature first, then by feature:
#   common/      board-independent adapters
#   ${BSP}/      board-specific adapters
# The fatfs_port / usb_port targets are declared here; the directories only
# group the sources.

# ---- FatFs port -----------------------------------------------------------
set(FATFS_PORT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/fatfs/exfuns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/fatfs/diskio.c)

add_library(fatfs_port STATIC EXCLUDE_FROM_ALL ${FATFS_PORT_SRC})

target_include_directories(fatfs_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/fatfs)
# FatFs core headers only. The core library is intentionally not a link
# dependency: diskio.c (here) and ff.c (core) reference each other, so the
# consumer must link fatfs_core before fatfs_port to keep static archive order.
target_include_directories(fatfs_port PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>
    # diskio.c may route a USB-MSC drive (guarded by FATFS_USB_MSC).
    ${CMAKE_CURRENT_SOURCE_DIR}/common/usb)
target_link_libraries(fatfs_port PUBLIC bsp)

# ---- USB port -------------------------------------------------------------
set(USB_PORT_SRC)

if(MODULE_USB_DEVICE_ENABLE)
    list(APPEND USB_PORT_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbd_conf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbd_desc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbd_storage_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbd_audio_if.c
        ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbd_cdc_if.c)
endif()

if(MODULE_USB_HOST_ENABLE)
    list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb/usbh_conf.c)

    if(MODULE_FATFS_ENABLE)
        list(APPEND USB_PORT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/common/usb/usbh_diskio.c)
    endif()
endif()

add_library(usb_port STATIC EXCLUDE_FROM_ALL ${USB_PORT_SRC})

target_include_directories(usb_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/usb
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb)
target_link_libraries(usb_port PUBLIC bsp)

# Vendor USB core headers only; the USB stack libraries are linked by the
# consumer (lib_usb_device / lib_usb_host) to preserve archive order.
if(TARGET usb_device)
    target_link_libraries(usb_port PRIVATE usb_device_inc)
    # The vendor core headers include usbd_conf.h, provided here.
    target_include_directories(usb_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb)
endif()
if(TARGET usb_host)
    target_link_libraries(usb_port PRIVATE usb_host_inc)
    target_include_directories(usb_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/usb)
endif()

# usbh_diskio.c includes "ff.h".
if(MODULE_FATFS_ENABLE)
    target_link_libraries(usb_port PRIVATE fatfs_core_inc)
endif()
