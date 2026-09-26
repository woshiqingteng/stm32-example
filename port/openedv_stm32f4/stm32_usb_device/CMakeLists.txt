# USB device port: board conf/desc plus one target per class.

add_library(stm32_usb_device_common_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_desc.c)

target_include_directories(stm32_usb_device_common_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_common_port PUBLIC bsp_delay drv_usb)
target_link_libraries(stm32_usb_device_common_port PRIVATE stm32_usb_device_inc)
# The vendor core headers include usbd_conf.h, provided here; expose the board
# conf directory through the module's include interface.
target_include_directories(stm32_usb_device_inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stm32_usb_device_cdc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_cdc_if.c)
target_include_directories(stm32_usb_device_cdc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_cdc_port PUBLIC stm32_usb_device_common_port)
target_link_libraries(stm32_usb_device_cdc_port PRIVATE stm32_usb_device_inc)

add_library(stm32_usb_device_audio_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_audio_if.c)
target_include_directories(stm32_usb_device_audio_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_audio_port PUBLIC
    stm32_usb_device_common_port bsp_codec bsp_sai)
target_link_libraries(stm32_usb_device_audio_port PRIVATE stm32_usb_device_inc)

add_library(stm32_usb_device_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_storage_if.c)
target_include_directories(stm32_usb_device_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_msc_port PUBLIC
    stm32_usb_device_common_port bsp_sdio)
target_link_libraries(stm32_usb_device_msc_port PRIVATE stm32_usb_device_inc)
