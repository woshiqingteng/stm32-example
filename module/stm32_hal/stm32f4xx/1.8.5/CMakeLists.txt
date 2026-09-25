# Vendor module: STM32F4xx HAL drivers.
#
# One STATIC library per HAL module (the "small class"). Big categories are
# grouped by comment blocks below. Everything is EXCLUDE_FROM_ALL: a module is
# compiled only when some built target depends on it. Each leaf exposes Inc/ and
# links the core / SoC; hal_all aggregates every leaf for whole-HAL consumers.

function(hal_library name)
    add_library(${name} STATIC EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
    target_link_libraries(${name} PUBLIC cmsis_core soc_${MCU_FAMILY})
    if(NOT name STREQUAL "hal_core")
        target_link_libraries(${name} PUBLIC hal_core)
    endif()
endfunction()

# ===== core =================================================================
hal_library(hal_core
    Src/stm32f4xx_hal.c
    Src/stm32f4xx_hal_cortex.c
    Src/stm32f4xx_hal_rcc.c
    Src/stm32f4xx_hal_rcc_ex.c
    Src/stm32f4xx_hal_pwr.c
    Src/stm32f4xx_hal_pwr_ex.c
    Src/stm32f4xx_hal_gpio.c
    Src/stm32f4xx_hal_exti.c
    Src/stm32f4xx_hal_dma.c
    Src/stm32f4xx_hal_dma_ex.c
    Src/stm32f4xx_ll_gpio.c
    Src/stm32f4xx_ll_dma.c
    Src/stm32f4xx_ll_rcc.c
    Src/stm32f4xx_ll_usart.c
    Src/stm32f4xx_ll_exti.c
    Src/stm32f4xx_ll_utils.c
    Src/stm32f4xx_ll_pwr.c)

# ===== time =================================================================
hal_library(hal_tim   Src/stm32f4xx_hal_tim.c Src/stm32f4xx_hal_tim_ex.c Src/stm32f4xx_ll_tim.c)
hal_library(hal_lptim Src/stm32f4xx_hal_lptim.c Src/stm32f4xx_ll_lptim.c)
hal_library(hal_rtc   Src/stm32f4xx_hal_rtc.c Src/stm32f4xx_hal_rtc_ex.c Src/stm32f4xx_ll_rtc.c)
hal_library(hal_iwdg  Src/stm32f4xx_hal_iwdg.c)
hal_library(hal_wwdg  Src/stm32f4xx_hal_wwdg.c)

# ===== comm =================================================================
hal_library(hal_uart  Src/stm32f4xx_hal_uart.c)
hal_library(hal_usart Src/stm32f4xx_hal_usart.c)
hal_library(hal_i2c   Src/stm32f4xx_hal_i2c.c Src/stm32f4xx_hal_i2c_ex.c Src/stm32f4xx_ll_i2c.c)
hal_library(hal_spi   Src/stm32f4xx_hal_spi.c Src/stm32f4xx_ll_spi.c)
hal_library(hal_i2s   Src/stm32f4xx_hal_i2s.c Src/stm32f4xx_hal_i2s_ex.c)
hal_library(hal_can   Src/stm32f4xx_hal_can.c)
hal_library(hal_eth   Src/stm32f4xx_hal_eth.c)
hal_library(hal_usb
    Src/stm32f4xx_hal_pcd.c
    Src/stm32f4xx_hal_pcd_ex.c
    Src/stm32f4xx_hal_hcd.c
    Src/stm32f4xx_ll_usb.c)
hal_library(hal_sd
    Src/stm32f4xx_hal_sd.c
    Src/stm32f4xx_hal_mmc.c
    Src/stm32f4xx_ll_sdmmc.c)
hal_library(hal_dcmi  Src/stm32f4xx_hal_dcmi.c Src/stm32f4xx_hal_dcmi_ex.c)
hal_library(hal_sai   Src/stm32f4xx_hal_sai.c Src/stm32f4xx_hal_sai_ex.c)
hal_library(hal_comm_misc
    Src/stm32f4xx_hal_irda.c
    Src/stm32f4xx_hal_smartcard.c
    Src/stm32f4xx_hal_cec.c
    Src/stm32f4xx_hal_smbus.c
    Src/stm32f4xx_hal_fmpsmbus.c
    Src/stm32f4xx_hal_fmpsmbus_ex.c
    Src/stm32f4xx_hal_fmpi2c.c
    Src/stm32f4xx_hal_fmpi2c_ex.c
    Src/stm32f4xx_hal_spdifrx.c
    Src/stm32f4xx_hal_dfsdm.c
    Src/stm32f4xx_hal_dsi.c
    Src/stm32f4xx_ll_fmpi2c.c)

# ===== analog ===============================================================
hal_library(hal_adc   Src/stm32f4xx_hal_adc.c Src/stm32f4xx_hal_adc_ex.c Src/stm32f4xx_ll_adc.c)
hal_library(hal_dac   Src/stm32f4xx_hal_dac.c Src/stm32f4xx_hal_dac_ex.c Src/stm32f4xx_ll_dac.c)

# ===== memory ===============================================================
hal_library(hal_flash Src/stm32f4xx_hal_flash.c Src/stm32f4xx_hal_flash_ex.c Src/stm32f4xx_hal_flash_ramfunc.c)
hal_library(hal_fmc
    Src/stm32f4xx_ll_fmc.c
    Src/stm32f4xx_ll_fsmc.c
    Src/stm32f4xx_hal_sdram.c
    Src/stm32f4xx_hal_sram.c
    Src/stm32f4xx_hal_nand.c
    Src/stm32f4xx_hal_nor.c
    Src/stm32f4xx_hal_pccard.c)
hal_library(hal_qspi  Src/stm32f4xx_hal_qspi.c)

# ===== display ==============================================================
hal_library(hal_dma2d Src/stm32f4xx_hal_dma2d.c Src/stm32f4xx_ll_dma2d.c)
hal_library(hal_ltdc  Src/stm32f4xx_hal_ltdc.c Src/stm32f4xx_hal_ltdc_ex.c)

# ===== crypto ===============================================================
hal_library(hal_crc   Src/stm32f4xx_hal_crc.c Src/stm32f4xx_ll_crc.c)
hal_library(hal_cryp  Src/stm32f4xx_hal_cryp.c Src/stm32f4xx_hal_cryp_ex.c)
hal_library(hal_hash  Src/stm32f4xx_hal_hash.c Src/stm32f4xx_hal_hash_ex.c)
hal_library(hal_rng   Src/stm32f4xx_hal_rng.c Src/stm32f4xx_ll_rng.c)

# ===== aggregate ============================================================
add_library(hal_all INTERFACE)
target_link_libraries(hal_all INTERFACE
    hal_core hal_tim hal_lptim hal_rtc hal_iwdg hal_wwdg
    hal_uart hal_usart hal_i2c hal_spi hal_i2s hal_can hal_eth
    hal_usb hal_sd hal_dcmi hal_sai hal_comm_misc
    hal_adc hal_dac hal_flash hal_fmc hal_qspi
    hal_dma2d hal_ltdc hal_crc hal_cryp hal_hash hal_rng)
