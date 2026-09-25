# SoC layer: selected by the board's MCU (MCU_VENDOR/MCU_FAMILY, set in
# cmake/board). MCU_TARGET_DIR is <vendor>/<family> under this directory.
add_subdirectory(${MCU_TARGET_DIR})
