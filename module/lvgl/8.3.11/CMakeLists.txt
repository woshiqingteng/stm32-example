file(GLOB_RECURSE LVGL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

# Silence the only two upstream files that trip the project's -Wall/-Wextra.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/extra/libs/qrcode/qrcodegen.c
    PROPERTIES COMPILE_OPTIONS "-Wno-type-limits")
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/misc/lv_tlsf.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")

add_library(lvgl STATIC EXCLUDE_FROM_ALL ${LVGL_SRC})

target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# lv_conf.h is resolved via "#include \"lv_conf.h\"" (LV_CONF_INCLUDE_SIMPLE);
# the app provides it on its own include path.
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

# HAL core: supplies the FPU ABI flags (and HAL headers). lvgl may use HAL;
# it must not link the SOC/CPU interfaces directly.
target_link_libraries(lvgl PUBLIC drv_core)
