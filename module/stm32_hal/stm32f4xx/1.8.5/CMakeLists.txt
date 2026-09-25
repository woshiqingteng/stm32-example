# Vendor module: STM32F4xx HAL drivers. Only the modules the project actually
# uses are compiled into bsp_drv; the linker keeps only the objects an image
# references. The source list is explicit (no GLOB) so new vendor files are not
# pulled in accidentally.
set(DRV_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_adc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dac_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dcmi_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_flash_ramfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_hcd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_ltdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_ltdc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_mmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_nand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_nor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pccard.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pcd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pcd_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_rtc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sai.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sai_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sdram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_sram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_hal_wwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_fsmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_sdmmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_usb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32f4xx_ll_utils.c)

add_library(bsp_drv STATIC ${DRV_SRC})

target_include_directories(bsp_drv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
target_link_libraries(bsp_drv PUBLIC cmsis_core soc_${MCU_FAMILY})
