add_freertos_app(lvgl_07_xbf_font
    SOURCE  main.c
            GUI_FONT/Font12.c
            GUI_FONT/Font24.c
            GUI_FONT/Font36.c
    BSP     nor lcd touch sdram
    LIB     lvgl text)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
