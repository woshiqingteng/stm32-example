add_library(lvgl_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_port_disp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_port_indev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_port_tick.c)

target_include_directories(lvgl_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# bsp_delay carries USE_FREERTOS (and freertos itself under the RTOS build).
target_link_libraries(lvgl_port
    PUBLIC lvgl bsp_lcd bsp_touch bsp_delay)
