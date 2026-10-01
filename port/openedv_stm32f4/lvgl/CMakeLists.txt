add_library(lvgl_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_port_fs.c)

target_include_directories(lvgl_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# lv_conf_common.h lives here (port), mirroring the lwIP port; lvgl sources
# resolve lv_conf.h (app) -> lv_conf_common.h (port).
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# bsp_delay carries USE_FREERTOS (and freertos itself under the RTOS build).
# fatfs_core provides ff.h and the FatFs symbols for the '0:' FS driver.
target_link_libraries(lvgl_port
    PUBLIC lvgl bsp_lcd bsp_touch bsp_delay m fatfs_core)
