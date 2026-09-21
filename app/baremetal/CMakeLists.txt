set(BAREMETAL_HAL_COMMON
    ${HAL_SRC_CORE} ${HAL_SRC_CORTEX} ${HAL_SRC_GPIO}
    ${HAL_SRC_RCC}  ${HAL_SRC_PWR}    ${HAL_SRC_FLASH}
    ${HAL_SRC_UART})
set(BAREMETAL_BSP_COMMON
    ${BSP_SRC_BSP} ${BSP_SRC_SYS} ${BSP_SRC_DELAY}
    ${BSP_SRC_USART} ${BSP_SRC_LED} ${BSP_SRC_KEY})

function(add_baremetal_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;HAL;BSP" ${ARGN})
    set(_target app_baremetal_${name})
    add_executable(${_target} ${ARG_SOURCES})
    target_link_libraries(${_target} PRIVATE bsp)

    target_sources(bsp           PRIVATE ${BAREMETAL_BSP_COMMON} ${ARG_BSP})
    target_sources(stm32f4xx_hal PRIVATE ${BAREMETAL_HAL_COMMON} ${ARG_HAL})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
