# Baremetal app tree: select the exposed modules here; cmake/func/flow.cmake
# only compiles the assembled source list. OS is baremetal, so no OS flag here.

include(${CMAKE_SOURCE_DIR}/cmake/func/flow.cmake)

set(BAREMETAL_COMMON_HAL HAL_CORE HAL_UART)
set(BAREMETAL_COMMON_BSP BSP_CORE)
set(BAREMETAL_COMMON TARGET_STM32F4 ${BAREMETAL_COMMON_HAL} ${BAREMETAL_COMMON_BSP})

# app_baremetal_add(NAME [MODULE...])
function(app_baremetal_add NAME)
    set(_srcs "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
    foreach(_m ${BAREMETAL_COMMON} ${ARGN})
        list(APPEND _srcs ${${_m}})
    endforeach()

    flow_compile(app_baremetal_${NAME}
        SOURCES ${_srcs}
        OUTDIR  ${CMAKE_BINARY_DIR}/baremetal/${NAME})
endfunction()

foreach(_app ${APPS})
    add_subdirectory(${_app})
endforeach()
