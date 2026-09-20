# Module selection. Real sources are vendored under <module>/<version>/.

set(MODULE_CMSIS_CORE_VERSION "v6.3.1" CACHE STRING "cmsis-core version")
set(MODULE_HAL_DRIVER_VERSION "v1.8.5" CACHE STRING "stm32f4xx-hal-driver version")

set(MODULE_CMSIS_CORE_DIR
    "${CMAKE_SOURCE_DIR}/module/cmsis-core/${MODULE_CMSIS_CORE_VERSION}")

# The version file provides HAL_DRIVER_SRC_DIR / HAL_DRIVER_INC_DIR and the
# HAL_MODULE_<name> -> source file mapping.
include(${CMAKE_SOURCE_DIR}/module/stm32f4xx-hal-driver/${MODULE_HAL_DRIVER_VERSION}/CMakeLists.txt)

# HAL modules this project exposes to applications.
set(HAL_EXPOSED_MODULES
    RCC RCC_EX FLASH FLASH_EX FLASH_RAMFUNC
    GPIO CORTEX PWR PWR_EX DMA DMA_EX
    UART TIM TIM_EX EXTI IWDG WWDG)

# Append the CMSIS core headers to the calling app (once).
macro(use_cmsis_core)
    if(NOT _CMSIS_ADDED)
        list(APPEND APP_SYSTEM_INCLUDES "${MODULE_CMSIS_CORE_DIR}/Include")
        set(_CMSIS_ADDED TRUE)
    endif()
endmacro()

# use_hal_driver(<MODULE>...) appends HAL core + the requested modules to the calling app.
# Only requested modules are compiled; names must be in HAL_EXPOSED_MODULES.
# Idempotent: HAL core is added once, modules are de-duplicated.
macro(use_hal_driver)
    if(NOT _HAL_CORE_ADDED)
        list(APPEND APP_SYSTEM_INCLUDES "${HAL_DRIVER_INC_DIR}")
        list(APPEND APP_DEFINES "USE_HAL_DRIVER")
        list(APPEND APP_SOURCES "${HAL_DRIVER_SRC_DIR}/stm32f4xx_hal.c")
        set(_hal_srcs "${HAL_DRIVER_SRC_DIR}/stm32f4xx_hal.c")
        set(_HAL_CORE_ADDED TRUE)
        set(_HAL_MODULES "")
    else()
        set(_hal_srcs "")
    endif()

    foreach(_m ${ARGV})
        if(NOT _m IN_LIST HAL_EXPOSED_MODULES)
            message(FATAL_ERROR "HAL module not exposed: ${_m}")
        endif()
        if(NOT _m IN_LIST _HAL_MODULES)
            list(APPEND _HAL_MODULES ${_m})
            foreach(_f ${HAL_MODULE_${_m}})
                list(APPEND APP_SOURCES "${HAL_DRIVER_SRC_DIR}/${_f}")
                list(APPEND _hal_srcs "${HAL_DRIVER_SRC_DIR}/${_f}")
            endforeach()
        endif()
    endforeach()

    if(_hal_srcs)
        set_source_files_properties(${_hal_srcs} PROPERTIES COMPILE_OPTIONS "-w")
    endif()
endmacro()
