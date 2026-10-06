# Board port for the CherryUSB DWC2 device stack.
#
# The cherryusb module sources resolve "usb_config.h" from this port dir
# (mirrors how the lwIP/lvgl ports inject their config headers).
target_include_directories(cherryusb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(cherryusb_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/cherryusb_glue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_device.c)

target_include_directories(cherryusb_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
# The board port owns the STM32 HAL dependency (the vendor glue in the cherryusb
# module only needs it to compile).
target_link_libraries(cherryusb_port PUBLIC
    cherryusb drv_usb)

# Force this object into the link so its strong HAL_PCD_MspInit overrides the
# ST HAL's weak empty stub (see cherryusb_glue_anchor()).
target_link_options(cherryusb_port INTERFACE -Wl,-u,cherryusb_glue_anchor)
