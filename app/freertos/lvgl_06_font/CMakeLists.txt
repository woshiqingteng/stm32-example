add_freertos_app(lvgl_06_font
    SOURCE  main.c
            GUI_FONT/Font14.c
    BSP     nor sdio lcd touch sdram
    LIB     lvgl text)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Only the font-update scratch lives in SRAM here; keep the internal SRAM
# for the LVGL buffers and the FreeRTOS heap.
target_compile_definitions(lib_malloc PRIVATE MEM1_MAX_SIZE=8192)
