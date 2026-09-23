set(DRV_SRC_CORE    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal.c CACHE INTERNAL "")
set(DRV_SRC_CORTEX  ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_cortex.c CACHE INTERNAL "")
set(DRV_SRC_GPIO    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_gpio.c CACHE INTERNAL "")
set(DRV_SRC_RCC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc_ex.c CACHE INTERNAL "")
set(DRV_SRC_PWR     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr_ex.c CACHE INTERNAL "")
set(DRV_SRC_FLASH   ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash_ex.c CACHE INTERNAL "")
set(DRV_SRC_UART    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_uart.c CACHE INTERNAL "")

set(DRV_SRC_DMA     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma_ex.c CACHE INTERNAL "")
set(DRV_SRC_ADC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_adc.c CACHE INTERNAL "")

set(DRV_SRC_TIM     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_tim.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_tim_ex.c CACHE INTERNAL "")
set(DRV_SRC_WDG     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_iwdg.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_wwdg.c CACHE INTERNAL "")
set(DRV_SRC_RTC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rtc.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rtc_ex.c CACHE INTERNAL "")
set(DRV_SRC_RNG     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rng.c CACHE INTERNAL "")
set(DRV_SRC_LTDC    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_ltdc.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_ltdc_ex.c CACHE INTERNAL "")
set(DRV_SRC_DMA2D   ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma2d.c CACHE INTERNAL "")
set(DRV_SRC_SRAM    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sram.c CACHE INTERNAL "")
set(DRV_SRC_SDRAM   ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sdram.c CACHE INTERNAL "")
set(DRV_SRC_FMC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_fmc.c CACHE INTERNAL "")

set(DRV_SRC_DAC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dac.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dac_ex.c CACHE INTERNAL "")
set(DRV_SRC_CAN     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_can.c CACHE INTERNAL "")
set(DRV_SRC_I2C     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_i2c.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_i2c_ex.c CACHE INTERNAL "")
set(DRV_SRC_SPI     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_spi.c CACHE INTERNAL "")
set(DRV_SRC_I2S     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_i2s.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_i2s_ex.c CACHE INTERNAL "")
set(DRV_SRC_SAI     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sai.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sai_ex.c CACHE INTERNAL "")
set(DRV_SRC_SD      ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sd.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_sdmmc.c CACHE INTERNAL "")
set(DRV_SRC_MMC     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_mmc.c CACHE INTERNAL "")
set(DRV_SRC_NAND    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_nand.c CACHE INTERNAL "")
set(DRV_SRC_NOR     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_nor.c CACHE INTERNAL "")
set(DRV_SRC_DCMI    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dcmi.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dcmi_ex.c CACHE INTERNAL "")
set(DRV_SRC_PCD     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pcd.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pcd_ex.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_usb.c CACHE INTERNAL "")
set(DRV_SRC_HCD     ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_hcd.c
                    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_usb.c CACHE INTERNAL "")

set(DRV_SRC_ALL
    ${DRV_SRC_CORE} ${DRV_SRC_CORTEX} ${DRV_SRC_GPIO} ${DRV_SRC_RCC} ${DRV_SRC_PWR} ${DRV_SRC_FLASH} ${DRV_SRC_UART} ${DRV_SRC_DMA} ${DRV_SRC_ADC} ${DRV_SRC_TIM} ${DRV_SRC_WDG} ${DRV_SRC_RTC} ${DRV_SRC_RNG} ${DRV_SRC_LTDC} ${DRV_SRC_DMA2D} ${DRV_SRC_SRAM} ${DRV_SRC_SDRAM} ${DRV_SRC_FMC} ${DRV_SRC_DAC} ${DRV_SRC_CAN} ${DRV_SRC_I2C} ${DRV_SRC_SPI} ${DRV_SRC_I2S} ${DRV_SRC_SAI} ${DRV_SRC_SD} ${DRV_SRC_MMC} ${DRV_SRC_NAND} ${DRV_SRC_NOR} ${DRV_SRC_DCMI} ${DRV_SRC_PCD} ${DRV_SRC_HCD})
list(REMOVE_DUPLICATES DRV_SRC_ALL)

add_library(stm32f4xx_hal STATIC ${DRV_SRC_ALL})

target_include_directories(stm32f4xx_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
target_link_libraries(stm32f4xx_hal PUBLIC ${TARGET_LIB} cmsis_core)
