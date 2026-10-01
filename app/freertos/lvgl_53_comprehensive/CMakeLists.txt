add_freertos_app(lvgl_53_comprehensive
    SOURCE  main.c
            gui/lv_mainstart.c
            gui/lv_calculator.c
            gui/lv_draw.c
            gui/lv_file.c
            gui/lv_meter.c
            gui/lv_qr.c
            gui/lv_scale.c
            gui/lv_setting.c
            gui/lv_shelf.c
            gui/image.c
            gui/img_hand.c
    BSP     nor sdio lcd touch sdram
    LIB     lvgl text)

# The app directory provides lv_conf.h; PUBLIC so lvgl.h consumers resolve it.
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The gui/ modules are imported verbatim from the ALIENTEK example; silence the
# warnings that trip the project's -Wall/-Wextra (as done for vendored LVGL).
set(_gui_warn -Wno-format -Wno-unused-variable -Wno-unused-parameter
              -Wno-maybe-uninitialized -Wno-missing-braces -Wno-sign-compare
              -Wno-shadow -Wno-unterminated-string-initialization
              -Wno-enum-compare -Wno-cast-function-type)
set_source_files_properties(
    gui/lv_mainstart.c gui/lv_calculator.c gui/lv_draw.c gui/lv_file.c
    gui/lv_meter.c gui/lv_qr.c gui/lv_scale.c gui/lv_setting.c gui/lv_shelf.c
    gui/image.c
    PROPERTIES COMPILE_OPTIONS "${_gui_warn}")
