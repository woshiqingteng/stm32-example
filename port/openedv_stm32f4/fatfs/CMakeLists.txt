set(FATFS_PORT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/exfuns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/diskio.c)

add_library(fatfs_port STATIC EXCLUDE_FROM_ALL ${FATFS_PORT_SRC})

target_include_directories(fatfs_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fatfs_port
    PUBLIC bsp_sdio bsp_nor
    PRIVATE fatfs_core_inc)

add_library(fatfs_stm32_usb_msc_port STATIC EXCLUDE_FROM_ALL ${FATFS_PORT_SRC})

target_include_directories(fatfs_stm32_usb_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fatfs_stm32_usb_msc_port PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../stm32_usb_host)
target_link_libraries(fatfs_stm32_usb_msc_port
    PUBLIC bsp_sdio bsp_nor stm32_usb_host_msc_port
    PRIVATE fatfs_core_inc)
target_compile_definitions(fatfs_stm32_usb_msc_port PRIVATE FATFS_USB_MSC=1)
