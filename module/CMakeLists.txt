set(MODULE_CMSIS_CORE_VERSION 6.3.1 CACHE STRING "cmsis_core version")
set(MODULE_HAL_VERSION        1.8.5 CACHE STRING "stm32f4xx-hal-driver version")

set(MODULE_CMSIS_CORE_ENABLE ON CACHE BOOL "Enable cmsis_core")
set(MODULE_HAL_ENABLE        ON CACHE BOOL "Enable stm32f4xx-hal-driver")

set(MODULE_FREERTOS_ENABLE OFF CACHE BOOL "Enable freertos")
if(OS STREQUAL "freertos")
    set(MODULE_FREERTOS_ENABLE ON CACHE BOOL "Enable freertos" FORCE)
endif()
set(MODULE_FREERTOS_VERSION 11.1.0 CACHE STRING "freertos version")

if(MODULE_CMSIS_CORE_ENABLE)
    add_subdirectory(cmsis_core/${MODULE_CMSIS_CORE_VERSION})
endif()

if(STM32_SERIES STREQUAL "stm32f4")
    set(HAL_MODULE_DIR stm32f4xx-hal-driver)
else()
    message(FATAL_ERROR "Unsupported STM32_SERIES: ${STM32_SERIES}")
endif()

if(MODULE_HAL_ENABLE)
    add_subdirectory(${HAL_MODULE_DIR}/${MODULE_HAL_VERSION})
endif()

if(MODULE_FREERTOS_ENABLE)
    add_subdirectory(freertos/${MODULE_FREERTOS_VERSION})
endif()
