# CherryUSB v1.6.1 source (vendored). Device-only build: the device core, the
# MSC / CDC-ACM / audio classes and the DWC2 device port. The host stack and
# the other ports are not compiled (the board's OTG_FS core is device-only in
# this bring-up). The FreeRTOS OSAL is used as the RTOS glue.
#
# The board port (port/<board>/cherryusb) adds "usb_config.h" to this target's
# include path and links the low-level glue (cherryusb_port).
add_library(cherryusb STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/core/usbd_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/class/msc/usbd_msc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/class/cdc/usbd_cdc_acm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/class/audio/usbd_audio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2/usb_dc_dwc2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2/usb_glue_st.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osal/usb_osal_freertos.c)

target_include_directories(cherryusb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/class
    ${CMAKE_CURRENT_SOURCE_DIR}/class/cdc
    ${CMAKE_CURRENT_SOURCE_DIR}/class/msc
    ${CMAKE_CURRENT_SOURCE_DIR}/class/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/class/hub
    ${CMAKE_CURRENT_SOURCE_DIR}/osal
    ${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2)

# drv_usb provides the STM32 HAL (HAL_PCD_* / the register definitions used by
# the DWC2 glue). PRIVATE: the HAL dependency is owned by the board port, not
# leaked to the applications. freertos provides the OSAL underneath
# usb_osal_freertos.c.
target_link_libraries(cherryusb PRIVATE drv_usb)
target_link_libraries(cherryusb PUBLIC drv_core)

if(TARGET freertos)
    target_link_libraries(cherryusb PUBLIC freertos)
endif()
