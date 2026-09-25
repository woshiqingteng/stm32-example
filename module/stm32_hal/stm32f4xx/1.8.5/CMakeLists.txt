set(DRV_PREFIX ${MCU_FAMILY})

function(drv_library name)
    add_library(${name} STATIC EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
    target_link_libraries(${name} PUBLIC cmsis_core soc_${MCU_FAMILY})
    if(NOT ${name} STREQUAL "drv_core")
        target_link_libraries(${name} PUBLIC drv_core)
    endif()
endfunction()

# core
drv_library(drv_core
    Src/${DRV_PREFIX}_hal.c
    Src/${DRV_PREFIX}_hal_cortex.c
    Src/${DRV_PREFIX}_hal_rcc.c
    Src/${DRV_PREFIX}_hal_rcc_ex.c
    Src/${DRV_PREFIX}_hal_pwr.c
    Src/${DRV_PREFIX}_hal_pwr_ex.c
    Src/${DRV_PREFIX}_hal_gpio.c
    Src/${DRV_PREFIX}_hal_exti.c
    Src/${DRV_PREFIX}_hal_dma.c
    Src/${DRV_PREFIX}_hal_dma_ex.c
    Src/${DRV_PREFIX}_ll_gpio.c
    Src/${DRV_PREFIX}_ll_dma.c
    Src/${DRV_PREFIX}_ll_rcc.c
    Src/${DRV_PREFIX}_ll_usart.c
    Src/${DRV_PREFIX}_ll_exti.c
    Src/${DRV_PREFIX}_ll_utils.c
    Src/${DRV_PREFIX}_ll_pwr.c)

# time
drv_library(drv_tim
    Src/${DRV_PREFIX}_hal_tim.c
    Src/${DRV_PREFIX}_hal_tim_ex.c
    Src/${DRV_PREFIX}_ll_tim.c)
drv_library(drv_rtc
    Src/${DRV_PREFIX}_hal_rtc.c
    Src/${DRV_PREFIX}_hal_rtc_ex.c
    Src/${DRV_PREFIX}_ll_rtc.c)
drv_library(drv_iwdg
    Src/${DRV_PREFIX}_hal_iwdg.c)
drv_library(drv_wwdg
    Src/${DRV_PREFIX}_hal_wwdg.c)

# comm
drv_library(drv_uart
    Src/${DRV_PREFIX}_hal_uart.c)
drv_library(drv_usart
    Src/${DRV_PREFIX}_hal_usart.c)
drv_library(drv_i2c
    Src/${DRV_PREFIX}_hal_i2c.c
    Src/${DRV_PREFIX}_hal_i2c_ex.c
    Src/${DRV_PREFIX}_ll_i2c.c)
drv_library(drv_spi
    Src/${DRV_PREFIX}_hal_spi.c
    Src/${DRV_PREFIX}_ll_spi.c)
drv_library(drv_i2s
    Src/${DRV_PREFIX}_hal_i2s.c
    Src/${DRV_PREFIX}_hal_i2s_ex.c)
drv_library(drv_can
    Src/${DRV_PREFIX}_hal_can.c)
drv_library(drv_eth
    Src/${DRV_PREFIX}_hal_eth.c)
drv_library(drv_usb
    Src/${DRV_PREFIX}_hal_pcd.c
    Src/${DRV_PREFIX}_hal_pcd_ex.c
    Src/${DRV_PREFIX}_hal_hcd.c
    Src/${DRV_PREFIX}_ll_usb.c)
drv_library(drv_dcmi
    Src/${DRV_PREFIX}_hal_dcmi.c
    Src/${DRV_PREFIX}_hal_dcmi_ex.c)
drv_library(drv_sai
    Src/${DRV_PREFIX}_hal_sai.c
    Src/${DRV_PREFIX}_hal_sai_ex.c)

# analog
drv_library(drv_adc
    Src/${DRV_PREFIX}_hal_adc.c
    Src/${DRV_PREFIX}_hal_adc_ex.c
    Src/${DRV_PREFIX}_ll_adc.c)
drv_library(drv_dac
    Src/${DRV_PREFIX}_hal_dac.c
    Src/${DRV_PREFIX}_hal_dac_ex.c
    Src/${DRV_PREFIX}_ll_dac.c)

# memory
drv_library(drv_flash
    Src/${DRV_PREFIX}_hal_flash.c
    Src/${DRV_PREFIX}_hal_flash_ex.c
    Src/${DRV_PREFIX}_hal_flash_ramfunc.c)
drv_library(drv_fmc
    Src/${DRV_PREFIX}_ll_fmc.c
    Src/${DRV_PREFIX}_ll_fsmc.c
    Src/${DRV_PREFIX}_hal_sdram.c
    Src/${DRV_PREFIX}_hal_sram.c
    Src/${DRV_PREFIX}_hal_nand.c
    Src/${DRV_PREFIX}_hal_nor.c
    Src/${DRV_PREFIX}_hal_pccard.c)
drv_library(drv_sd
    Src/${DRV_PREFIX}_hal_sd.c
    Src/${DRV_PREFIX}_hal_mmc.c
    Src/${DRV_PREFIX}_ll_sdmmc.c)

# display
drv_library(drv_dma2d
    Src/${DRV_PREFIX}_hal_dma2d.c
    Src/${DRV_PREFIX}_ll_dma2d.c)
drv_library(drv_ltdc
    Src/${DRV_PREFIX}_hal_ltdc.c
    Src/${DRV_PREFIX}_hal_ltdc_ex.c)

# crypto
drv_library(drv_crc
    Src/${DRV_PREFIX}_hal_crc.c
    Src/${DRV_PREFIX}_ll_crc.c)
drv_library(drv_rng
    Src/${DRV_PREFIX}_hal_rng.c
    Src/${DRV_PREFIX}_ll_rng.c)

# all
add_library(drv_all INTERFACE)
target_link_libraries(drv_all INTERFACE
    drv_core
    drv_tim drv_rtc drv_iwdg drv_wwdg
    drv_uart drv_usart drv_i2c drv_spi drv_i2s drv_can drv_eth
    drv_usb drv_dcmi drv_sai
    drv_adc drv_dac
    drv_flash drv_fmc drv_sd
    drv_dma2d drv_ltdc
    drv_crc drv_rng)
