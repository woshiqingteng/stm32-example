# FatFs port: the board diskio routing plus the board-independent exfuns helper.
# FatFs core headers are exposed, but the core library is intentionally not a
# link dependency: diskio.c (here) and ff.c (core) reference each other, so the
# consumer must link fatfs_core before the port to keep static archive order.
set(FATFS_PORT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/exfuns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/diskio.c)

add_library(fatfs_port STATIC EXCLUDE_FROM_ALL ${FATFS_PORT_SRC})

target_include_directories(fatfs_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fatfs_port PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(fatfs_port PUBLIC bsp_sdio bsp_nor)

# Variant that routes FatFs drive 2 to the USB host MSC glue
# (../stm32_usb_host/usbh_diskio.c). Selected by 57_usb_host_msc.
add_library(fatfs_usbmsc_port STATIC EXCLUDE_FROM_ALL ${FATFS_PORT_SRC})

target_include_directories(fatfs_usbmsc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fatfs_usbmsc_port PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_CURRENT_SOURCE_DIR}/../stm32_usb_host)
target_link_libraries(fatfs_usbmsc_port PUBLIC bsp_sdio bsp_nor stm32_usb_host_msc_port)
target_compile_definitions(fatfs_usbmsc_port PRIVATE FATFS_USB_MSC=1)
