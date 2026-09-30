add_freertos_app(lv_29_keyboard
    SOURCE  main.c
    BSP     lcd touch sdram
    LIB     lvgl)

# The app directory provides lv_conf.h; PUBLIC so every target that includes
# lvgl.h (lvgl itself, lvgl_port) resolves the config.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
