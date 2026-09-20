# Module version selection. Real sources are vendored under <module>/<version>/.

set(MODULE_CMSIS_CORE_VERSION "v6.3.1" CACHE STRING "cmsis-core version")
set(MODULE_HAL_DRIVER_VERSION "v1.8.5" CACHE STRING "stm32f4xx-hal-driver version")

set(MODULE_CMSIS_CORE_DIR
    "${CMAKE_SOURCE_DIR}/module/cmsis-core/${MODULE_CMSIS_CORE_VERSION}")

include(${CMAKE_SOURCE_DIR}/module/stm32f4xx-hal-driver/${MODULE_HAL_DRIVER_VERSION}/CMakeLists.txt)

# Append the CMSIS core headers to the calling app.
macro(use_cmsis_core)
    list(APPEND APP_SYSTEM_INCLUDES "${MODULE_CMSIS_CORE_DIR}/Include")
endmacro()
