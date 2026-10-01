add_freertos_app(lvgl_49_qrgen
    SOURCE  main.c
            GUI_FONT/myFont14.c
            GUI_FONT/myFont24.c
    BSP     lcd touch sdram
    LIB     lvgl)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
