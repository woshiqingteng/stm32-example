set(HAL_SRC_CORE    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal.c CACHE INTERNAL "")
set(HAL_SRC_CORTEX  ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_cortex.c CACHE INTERNAL "")
set(HAL_SRC_GPIO    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_gpio.c CACHE INTERNAL "")
set(HAL_SRC_RCC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc_ex.c CACHE INTERNAL "")
set(HAL_SRC_PWR     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr_ex.c CACHE INTERNAL "")
set(HAL_SRC_FLASH   ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash_ex.c CACHE INTERNAL "")
set(HAL_SRC_UART    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_uart.c CACHE INTERNAL "")

set(HAL_SRC_DMA     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma_ex.c CACHE INTERNAL "")
set(HAL_SRC_ADC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_adc.c CACHE INTERNAL "")

add_library(stm32f4xx_hal STATIC)

target_include_directories(stm32f4xx_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
target_link_libraries(stm32f4xx_hal PRIVATE target_stm32f4 cmsis_core)
