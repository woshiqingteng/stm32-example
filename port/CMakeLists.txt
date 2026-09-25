# Port layer: vendor-stack -> hardware adaptation.
# Layout groups by nature first, then by feature:
#   common/      board-independent adapters
#   ${BSP}/      board-specific adapters
# The targets are declared here; the directories only group the sources.

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
    ${CMAKE_CURRENT_SOURCE_DIR}/common/stm32_usb_host)
target_link_libraries(fatfs_port PUBLIC bsp_sdio bsp_norflash)

# ---- USB device port (per class) ------------------------------------------
add_library(stm32_usb_device_common_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device/usbd_desc.c)

target_include_directories(stm32_usb_device_common_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device)
target_link_libraries(stm32_usb_device_common_port PUBLIC bsp_core drv_usb)
target_link_libraries(stm32_usb_device_common_port PRIVATE stm32_usb_device_inc)
# The vendor core headers include usbd_conf.h, provided here.
target_include_directories(stm32_usb_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device)

add_library(stm32_usb_device_cdc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device/usbd_cdc_if.c)
target_include_directories(stm32_usb_device_cdc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device)
target_link_libraries(stm32_usb_device_cdc_port PUBLIC stm32_usb_device_common_port)
target_link_libraries(stm32_usb_device_cdc_port PRIVATE stm32_usb_device_inc)

add_library(stm32_usb_device_audio_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device/usbd_audio_if.c)
target_include_directories(stm32_usb_device_audio_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device)
target_link_libraries(stm32_usb_device_audio_port PUBLIC
    stm32_usb_device_common_port bsp_es8388 bsp_sai)
target_link_libraries(stm32_usb_device_audio_port PRIVATE stm32_usb_device_inc)

add_library(stm32_usb_device_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device/usbd_storage_if.c)
target_include_directories(stm32_usb_device_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_device)
target_link_libraries(stm32_usb_device_msc_port PUBLIC
    stm32_usb_device_common_port bsp_sdio)
target_link_libraries(stm32_usb_device_msc_port PRIVATE stm32_usb_device_inc)

# ---- USB host port --------------------------------------------------------
add_library(stm32_usb_host_common_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_host/usbh_conf.c)
target_include_directories(stm32_usb_host_common_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_host)
target_link_libraries(stm32_usb_host_common_port PUBLIC bsp_core bsp_pcf8574 drv_usb)
target_link_libraries(stm32_usb_host_common_port PRIVATE stm32_usb_host_inc)
# The vendor core headers include usbh_conf.h, provided here.
target_include_directories(stm32_usb_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_host)

add_library(stm32_usb_host_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/common/stm32_usb_host/usbh_diskio.c)
target_include_directories(stm32_usb_host_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/stm32_usb_host
    ${CMAKE_CURRENT_SOURCE_DIR}/${BSP}/stm32_usb_host)
target_link_libraries(stm32_usb_host_msc_port PUBLIC stm32_usb_host_common_port)
target_link_libraries(stm32_usb_host_msc_port PRIVATE stm32_usb_host_inc fatfs_core_inc)
