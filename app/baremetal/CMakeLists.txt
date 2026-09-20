# Baremetal experiments: common setup lives here, each app adds only its extras.

set(BAREMETAL_COMMON_HAL RCC CORTEX GPIO UART)
set(BAREMETAL_COMMON_BSP sys delay usart led key)

macro(baremetal_begin NAME)
    app_begin(${NAME})
    use_target()
    use_cmsis_core()
    use_hal_driver(${BAREMETAL_COMMON_HAL})
    use_bsp(${BAREMETAL_COMMON_BSP})
endmacro()

add_subdirectory(01_led)
