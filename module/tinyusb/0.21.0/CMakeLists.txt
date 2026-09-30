# TinyUSB 0.21.0 source (vendored). NOT wired into the build yet. This file is
# only a starting point for a future port.

file(GLOB_RECURSE TINYUSB_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(tinyusb STATIC EXCLUDE_FROM_ALL ${TINYUSB_SRC})

target_include_directories(tinyusb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# tusb_config.h is provided by the port/app (TUSB_OPT_*); the real port
# (device/host selection, MCU port sources) is deferred.
target_link_libraries(tinyusb PUBLIC drv_core)
