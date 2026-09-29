add_freertos_app(lv_29_keyboard
    SOURCE  main.c
    BSP     lcd touch sdram
    LIB     lvgl)

# The app directory provides lv_conf.h for the lvgl target.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
