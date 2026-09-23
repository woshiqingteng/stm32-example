# Baremetal application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

set(BAREMETAL_HAL_COMMON
    ${HAL_SRC_CORE} ${HAL_SRC_CORTEX} ${HAL_SRC_GPIO}
    ${HAL_SRC_RCC}  ${HAL_SRC_PWR}    ${HAL_SRC_FLASH}
    ${HAL_SRC_UART} ${HAL_SRC_DMA})
set(BAREMETAL_BSP_COMMON
    ${BSP_SRC_BSP} ${BSP_SRC_SYS} ${BSP_SRC_DELAY}
    ${BSP_SRC_USART} ${BSP_SRC_LED} ${BSP_SRC_KEY})

# Apps list only BSP and LIB components; the matching HAL sources are derived.
# Linker script selection is owned by the target layer (MCU_LD_SCRIPT property).
function(add_baremetal_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;BSP;LIB" ${ARGN})
    set(_target app_baremetal_${name})
    add_executable(${_target} ${ARG_SOURCES})

    bsp_resolve_hal(_hal ${ARG_BSP})
    lib_resolve(_libs _lib_hal _lib_bsp ${ARG_LIB})

    target_link_libraries(${_target} PRIVATE bsp ${_libs})
    target_sources(bsp           PRIVATE ${BAREMETAL_BSP_COMMON} ${ARG_BSP} ${_lib_bsp})
    target_sources(stm32f4xx_hal PRIVATE ${BAREMETAL_HAL_COMMON} ${_hal} ${_lib_hal})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
