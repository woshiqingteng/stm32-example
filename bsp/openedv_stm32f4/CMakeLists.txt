# Board support package: every board driver in this directory is compiled into
# the bsp library. Applications link bsp (and optionally LIB components); the
# linker keeps only the objects an image actually references, so no per-app
# component selection or HAL mapping is needed here.
file(GLOB BSP_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(bsp STATIC ${BSP_SRC})

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp PUBLIC stm32f4xx_hal ${TARGET_LIB})
