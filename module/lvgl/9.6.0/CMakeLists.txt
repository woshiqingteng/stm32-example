# LVGL v9.6.0 source (vendored). NOT wired into the build yet:
# module/lvgl/CMakeLists.txt still selects 8.3.11. This file is only a starting
# point for a future port.

file(GLOB_RECURSE LVGL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(lvgl STATIC EXCLUDE_FROM_ALL ${LVGL_SRC})

target_include_directories(lvgl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# lv_conf.h is resolved via "#include \"lv_conf.h\"" (LV_CONF_INCLUDE_SIMPLE);
# the app provides it on its own include path.
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)

# HAL core: supplies the FPU ABI flags (and HAL headers). lvgl may use HAL;
# it must not link the SOC/CPU interfaces directly.
target_link_libraries(lvgl PUBLIC drv_core)
