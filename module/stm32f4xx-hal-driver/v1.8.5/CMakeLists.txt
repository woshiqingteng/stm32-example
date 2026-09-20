# STM32F4xx HAL driver (v1.8.5).
# Exposed modules are CACHE INTERNAL variables carrying their source files.

set(HAL_INC "${CMAKE_CURRENT_LIST_DIR}/Inc" CACHE INTERNAL "" FORCE)
set(HAL_SRC "${CMAKE_CURRENT_LIST_DIR}/Src" CACHE INTERNAL "" FORCE)

set(HAL_EXPOSED_MODULES HAL_CORE HAL_DMA HAL_UART HAL_TIM HAL_WDG CACHE INTERNAL "" FORCE)

set(HAL_CORE
    ${HAL_SRC}/stm32f4xx_hal.c
    ${HAL_SRC}/stm32f4xx_hal_cortex.c
    ${HAL_SRC}/stm32f4xx_hal_rcc.c
    ${HAL_SRC}/stm32f4xx_hal_rcc_ex.c
    ${HAL_SRC}/stm32f4xx_hal_flash.c
    ${HAL_SRC}/stm32f4xx_hal_flash_ex.c
    ${HAL_SRC}/stm32f4xx_hal_flash_ramfunc.c
    ${HAL_SRC}/stm32f4xx_hal_pwr.c
    ${HAL_SRC}/stm32f4xx_hal_pwr_ex.c
    ${HAL_SRC}/stm32f4xx_hal_gpio.c
    ${HAL_SRC}/stm32f4xx_hal_exti.c
    CACHE INTERNAL "" FORCE)
set(HAL_DMA  ${HAL_SRC}/stm32f4xx_hal_dma.c ${HAL_SRC}/stm32f4xx_hal_dma_ex.c CACHE INTERNAL "" FORCE)
set(HAL_UART ${HAL_SRC}/stm32f4xx_hal_uart.c CACHE INTERNAL "" FORCE)
set(HAL_TIM  ${HAL_SRC}/stm32f4xx_hal_tim.c ${HAL_SRC}/stm32f4xx_hal_tim_ex.c CACHE INTERNAL "" FORCE)
set(HAL_WDG  ${HAL_SRC}/stm32f4xx_hal_iwdg.c ${HAL_SRC}/stm32f4xx_hal_wwdg.c CACHE INTERNAL "" FORCE)

# Union of all HAL sources; the app flow disables warnings for these.
set(HAL_SOURCES ${HAL_CORE} ${HAL_DMA} ${HAL_UART} ${HAL_TIM} ${HAL_WDG}
    CACHE INTERNAL "" FORCE)
