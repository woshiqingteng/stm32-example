add_freertos_app(lvgl_45_jpeg
    SOURCE  main.c
    BSP     sdio lcd touch sdram
    LIB     lvgl fatfs)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
