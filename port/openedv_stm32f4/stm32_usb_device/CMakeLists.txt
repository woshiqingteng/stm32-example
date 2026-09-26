target_include_directories(stm32_usb_device_inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stm32_usb_device_conf_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_desc.c)

target_include_directories(stm32_usb_device_conf_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_conf_port PUBLIC bsp_delay drv_usb stm32_usb_device)

add_library(stm32_usb_device_cdc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_cdc_if.c)
target_include_directories(stm32_usb_device_cdc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_cdc_port PUBLIC stm32_usb_device_conf_port)

add_library(stm32_usb_device_audio_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_audio_if.c)
target_include_directories(stm32_usb_device_audio_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_audio_port PUBLIC
    stm32_usb_device_conf_port bsp_codec bsp_sai)

add_library(stm32_usb_device_msc_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/usbd_storage_if.c)
target_include_directories(stm32_usb_device_msc_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(stm32_usb_device_msc_port PUBLIC
    stm32_usb_device_conf_port bsp_sdio bsp_nor)
