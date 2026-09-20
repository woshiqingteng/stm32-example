# Module version selection. Each version file defines the exposed module variables.

set(MODULE_CMSIS_CORE_VERSION "v6.3.1" CACHE STRING "cmsis-core version")
set(MODULE_HAL_DRIVER_VERSION "v1.8.5" CACHE STRING "stm32f4xx-hal-driver version")

include("${CMAKE_CURRENT_LIST_DIR}/cmsis-core/${MODULE_CMSIS_CORE_VERSION}/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/stm32f4xx-hal-driver/${MODULE_HAL_DRIVER_VERSION}/CMakeLists.txt")
