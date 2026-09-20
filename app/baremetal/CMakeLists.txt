# Baremetal app tree: declare common modules here; link/flash mechanics come
# from cmake/func/firmware.cmake.

include(${CMAKE_SOURCE_DIR}/cmake/func/firmware.cmake)

set(BAREMETAL_COMMON_HAL HAL_CORE HAL_UART)
set(BAREMETAL_COMMON_BSP BSP_CORE)
set(BAREMETAL_COMMON TARGET_STM32F4 ${BAREMETAL_COMMON_HAL} ${BAREMETAL_COMMON_BSP})

function(app_baremetal_add NAME)
    add_firmware(app_baremetal_${NAME}
        OUTDIR  ${CMAKE_BINARY_DIR}/baremetal/${NAME}
        MODULES ${BAREMETAL_COMMON} ${ARGN}
        DEFS    BSP_SUPPORT_OS=${BSP_SUPPORT_OS})
endfunction()

foreach(_app ${APPS})
    add_subdirectory(${_app})
endforeach()
