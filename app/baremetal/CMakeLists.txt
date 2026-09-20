# Baremetal app tree. Common modules are declared here; each app adds only extras.

set(BAREMETAL_COMMON TARGET_STM32F4 HAL_CORE HAL_UART BSP_CORE)

# Thin wrapper: defer the full "link -> flash" flow to cmake/func/flow.cmake.
function(app_baremetal_add NAME)
    flow_app_baremetal(${NAME} ${ARGN})
endfunction()

add_subdirectory(01_led)
