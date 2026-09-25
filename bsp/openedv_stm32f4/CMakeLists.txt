# Board support package: every board driver in this directory is compiled into
# the bsp library. Applications link bsp (and optionally LIB components); the
# linker keeps only the objects an image actually references.
file(GLOB BSP_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(bsp STATIC ${BSP_SRC})

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp PUBLIC bsp_drv)

# When the FreeRTOS module is enabled, the board layer uses the RTOS timing
# services (delay/tick in delay.c). The OS selection is driven here, not by the
# application, so bsp stays self-contained.
if(MODULE_FREERTOS_ENABLE)
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
endif()
