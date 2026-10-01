add_freertos_app(lvgl_40_img_lib
    SOURCE  main.c
            image.c
    BSP     nor sdio lcd touch sdram
    LIB     lvgl fatfs)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
