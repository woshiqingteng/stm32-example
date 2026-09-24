# Vendor module: STM32F4xx HAL drivers. The full driver source set is compiled
# into bsp_drv; the linker keeps only the objects an image references.
file(GLOB DRV_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c)
# Drop vendor *_template.c files (msp / timebase) - they must be copied in, not
# compiled as-is.
list(FILTER DRV_SRC EXCLUDE REGEX "_template\\.c$")

add_library(bsp_drv STATIC ${DRV_SRC})

target_include_directories(bsp_drv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
target_link_libraries(bsp_drv PUBLIC cmsis_core target_${MCU_FAMILY})
