# Baremetal app tree. Common modules are declared here; each app adds only extras.

include(${CMAKE_SOURCE_DIR}/cmake/func/flow.cmake)

set(BAREMETAL_COMMON_HAL HAL_CORE HAL_UART)
set(BAREMETAL_COMMON_BSP BSP_CORE)
set(BAREMETAL_COMMON TARGET_STM32F4 ${BAREMETAL_COMMON_HAL} ${BAREMETAL_COMMON_BSP})

# Thin wrapper: defer the full "link -> flash" flow to cmake/func/flow.cmake.
function(app_baremetal_add NAME)
    flow_app_baremetal(${NAME} ${ARGN})
endfunction()

foreach(_app ${APPS})
    add_subdirectory(${_app})
endforeach()
