add_freertos_app(lvgl_13_btnmatrix
    SOURCE  main.c
            img_user.c
    BSP     lcd touch sdram
    LIB     lvgl)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
