# STM32F4xx HAL driver (v1.8.5).
# Exposed modules are variables carrying their source files.

set(HAL_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(HAL_INC "${HAL_DIR}/Inc")
set(HAL_SRC "${HAL_DIR}/Src")

set(HAL_EXPOSED_MODULES HAL_CORE HAL_DMA HAL_UART HAL_TIM HAL_WDG)

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
    ${HAL_SRC}/stm32f4xx_hal_exti.c)
set(HAL_DMA  ${HAL_SRC}/stm32f4xx_hal_dma.c ${HAL_SRC}/stm32f4xx_hal_dma_ex.c)
set(HAL_UART ${HAL_SRC}/stm32f4xx_hal_uart.c)
set(HAL_TIM  ${HAL_SRC}/stm32f4xx_hal_tim.c ${HAL_SRC}/stm32f4xx_hal_tim_ex.c)
set(HAL_WDG  ${HAL_SRC}/stm32f4xx_hal_iwdg.c ${HAL_SRC}/stm32f4xx_hal_wwdg.c)

# HAL sources are third-party: disable warnings for them.
set_source_files_properties(${HAL_CORE} ${HAL_DMA} ${HAL_UART} ${HAL_TIM} ${HAL_WDG}
    PROPERTIES COMPILE_OPTIONS "-w")
