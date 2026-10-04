function(bsp_library name)
    add_library(${name} STATIC EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PUBLIC drv_core)
endfunction()

# core
bsp_library(bsp_sys
    sys.c)
bsp_library(bsp_delay
    delay.c)
if(OS STREQUAL "freertos")
    target_compile_definitions(bsp_delay PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp_delay PUBLIC freertos)
endif()
bsp_library(bsp_led
    led.c)
bsp_library(bsp_key
    key.c)
bsp_library(bsp_usart
    usart.c)
target_link_libraries(bsp_usart PUBLIC drv_uart PRIVATE common)
bsp_library(bsp_core
    bsp.c)
target_link_libraries(bsp_core PUBLIC bsp_sys bsp_delay bsp_led bsp_key bsp_usart)

# time
bsp_library(bsp_btim
    btim.c)
target_link_libraries(bsp_btim PUBLIC drv_tim)
bsp_library(bsp_gtim
    gtim.c)
target_link_libraries(bsp_gtim PUBLIC drv_tim)
bsp_library(bsp_atim
    atim.c)
target_link_libraries(bsp_atim PUBLIC drv_tim)
bsp_library(bsp_tim
    tim.c)
target_link_libraries(bsp_tim PUBLIC drv_tim)
bsp_library(bsp_rtc
    rtc.c)
target_link_libraries(bsp_rtc PUBLIC drv_rtc)
bsp_library(bsp_wdg
    wdg.c)
target_link_libraries(bsp_wdg PUBLIC drv_iwdg drv_wwdg)

# comm
bsp_library(bsp_i2c
    i2c.c)
target_link_libraries(bsp_i2c PUBLIC drv_i2c)
bsp_library(bsp_spi
    spi.c)
target_link_libraries(bsp_spi PUBLIC drv_spi)
bsp_library(bsp_io_expand
    io_expand.c)
target_link_libraries(bsp_io_expand PUBLIC bsp_i2c)
bsp_library(bsp_can
    can.c)
target_link_libraries(bsp_can PUBLIC drv_can)
bsp_library(bsp_rs485
    rs485.c)
target_link_libraries(bsp_rs485 PUBLIC bsp_io_expand drv_uart)
bsp_library(bsp_wireless_nrf24l01
    wireless_nrf24l01.c)
target_link_libraries(bsp_wireless_nrf24l01 PUBLIC bsp_spi)
bsp_library(bsp_wireless
    wireless.c)
target_link_libraries(bsp_wireless PUBLIC bsp_wireless_nrf24l01)
bsp_library(bsp_ir
    ir.c)
target_link_libraries(bsp_ir PUBLIC drv_tim)

# analog
bsp_library(bsp_adc
    adc.c)
target_link_libraries(bsp_adc PUBLIC drv_adc)
bsp_library(bsp_dac
    dac.c)
target_link_libraries(bsp_dac PUBLIC drv_dac drv_tim m)
bsp_library(bsp_pwmdac
    pwmdac.c)
target_link_libraries(bsp_pwmdac PUBLIC bsp_dac drv_tim)

# memory
bsp_library(bsp_internal_flash
    internal_flash.c)
target_link_libraries(bsp_internal_flash PUBLIC drv_flash)
bsp_library(bsp_sdram
    sdram.c)
target_link_libraries(bsp_sdram PUBLIC drv_fmc)
bsp_library(bsp_nand_mt29f4g08
    nand_mt29f4g08.c)
bsp_library(bsp_nand
    nand.c)
target_link_libraries(bsp_nand PUBLIC drv_fmc bsp_nand_mt29f4g08)
bsp_library(bsp_ftl
    ftl.c)
target_link_libraries(bsp_ftl PUBLIC bsp_nand)
bsp_library(bsp_nor_w25q256jv
    nor_w25q256jv.c)
target_link_libraries(bsp_nor_w25q256jv PUBLIC bsp_spi bsp_delay)
bsp_library(bsp_nor
    nor.c)
target_link_libraries(bsp_nor PUBLIC bsp_nor_w25q256jv)
bsp_library(bsp_eeprom_at24c02
    eeprom_at24c02.c)
target_link_libraries(bsp_eeprom_at24c02 PUBLIC bsp_i2c bsp_delay)
bsp_library(bsp_eeprom
    eeprom.c)
target_link_libraries(bsp_eeprom PUBLIC bsp_eeprom_at24c02)
bsp_library(bsp_sdio
    sdio.c)
target_link_libraries(bsp_sdio PUBLIC drv_sd)

# display
bsp_library(bsp_lcd_rgb
    lcd_rgb.c)
target_link_libraries(bsp_lcd_rgb PUBLIC drv_ltdc drv_dma2d)
bsp_library(bsp_lcd
    lcd.c)
target_link_libraries(bsp_lcd PUBLIC bsp_lcd_rgb)
bsp_library(bsp_oled_ssd1306
    oled_ssd1306.c)
bsp_library(bsp_oled
    oled.c)
target_link_libraries(bsp_oled PUBLIC bsp_oled_ssd1306)
bsp_library(bsp_touch_gt9xxx
    touch_gt9xxx.c)
target_link_libraries(bsp_touch_gt9xxx PUBLIC bsp_delay)
bsp_library(bsp_touch
    touch.c)
target_link_libraries(bsp_touch PUBLIC bsp_touch_gt9xxx bsp_lcd)
bsp_library(bsp_tpad
    tpad.c)
target_link_libraries(bsp_tpad PUBLIC drv_tim)

# multimedia
bsp_library(bsp_dcmi
    dcmi.c)
target_link_libraries(bsp_dcmi PUBLIC drv_dcmi bsp_ov5640)
bsp_library(bsp_ov5640
    ov5640.c)
target_link_libraries(bsp_ov5640 PUBLIC bsp_io_expand)
bsp_library(bsp_codec_es8388
    codec_es8388.c)
target_link_libraries(bsp_codec_es8388 PUBLIC bsp_i2c bsp_delay)
bsp_library(bsp_codec
    codec.c)
target_link_libraries(bsp_codec PUBLIC bsp_codec_es8388)
bsp_library(bsp_sai
    sai.c)
target_link_libraries(bsp_sai PUBLIC drv_sai)

# crypto
bsp_library(bsp_rng
    rng.c)
target_link_libraries(bsp_rng PUBLIC drv_rng)

# network
bsp_library(bsp_eth_phy_yt8512c
    eth_phy_yt8512c.c)
bsp_library(bsp_eth_phy
    eth_phy.c)
target_link_libraries(bsp_eth_phy PUBLIC bsp_eth_phy_yt8512c)
bsp_library(bsp_eth
    eth.c)
target_link_libraries(bsp_eth PUBLIC bsp_eth_phy bsp_io_expand bsp_delay drv_eth)

# sensor
bsp_library(bsp_als_ap3216c
    als_ap3216c.c)
target_link_libraries(bsp_als_ap3216c PUBLIC bsp_i2c)
bsp_library(bsp_als
    als.c)
target_link_libraries(bsp_als PUBLIC bsp_als_ap3216c)
bsp_library(bsp_imu_sh3001
    imu_sh3001.c)
target_link_libraries(bsp_imu_sh3001 PUBLIC bsp_i2c)
bsp_library(bsp_imu
    imu.c)
target_link_libraries(bsp_imu PUBLIC bsp_imu_sh3001 m bsp_delay)
bsp_library(bsp_mag_st480mc
    mag_st480mc.c)
target_link_libraries(bsp_mag_st480mc PUBLIC bsp_i2c)
bsp_library(bsp_mag
    mag.c)
target_link_libraries(bsp_mag PUBLIC bsp_mag_st480mc)
bsp_library(bsp_temp_ds18b20
    temp_ds18b20.c)
bsp_library(bsp_temp
    temp.c)
target_link_libraries(bsp_temp PUBLIC bsp_temp_ds18b20)
bsp_library(bsp_humi_dht11
    humi_dht11.c)
bsp_library(bsp_humi
    humi.c)
target_link_libraries(bsp_humi PUBLIC bsp_humi_dht11)

# misc
bsp_library(bsp_exti
    exti.c)
target_link_libraries(bsp_exti PUBLIC bsp_key)
bsp_library(bsp_pwr
    pwr.c)
target_link_libraries(bsp_pwr PUBLIC bsp_exti)
bsp_library(bsp_iap
    iap.c)
target_link_libraries(bsp_iap PUBLIC bsp_usart drv_flash)

# all
add_library(bsp_all INTERFACE)
target_link_libraries(bsp_all INTERFACE
    bsp_core
    bsp_btim bsp_gtim bsp_atim bsp_rtc bsp_wdg
    bsp_i2c bsp_spi bsp_io_expand
    bsp_can bsp_rs485 bsp_wireless bsp_ir
    bsp_adc bsp_dac bsp_pwmdac
    bsp_internal_flash bsp_sdram bsp_nand bsp_ftl bsp_nor bsp_eeprom bsp_sdio
    bsp_lcd bsp_lcd_rgb bsp_oled bsp_touch bsp_tpad
    bsp_dcmi bsp_ov5640 bsp_codec bsp_sai
    bsp_rng
    bsp_eth
    bsp_als_ap3216c bsp_als bsp_imu bsp_mag_st480mc bsp_mag bsp_temp_ds18b20 bsp_temp bsp_humi_dht11 bsp_humi
    bsp_exti bsp_pwr bsp_iap)

function(bsp_resolve out)
    set(_r bsp_core)
    foreach(_n ${ARGN})
        if(_n STREQUAL "all")
            list(APPEND _r bsp_all)
        elseif(_n STREQUAL "core")
            # already included
        elseif(TARGET bsp_${_n})
            list(APPEND _r bsp_${_n})
        else()
            message(FATAL_ERROR "Unknown BSP component: '${_n}'")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _r)
    set(${out} ${_r} PARENT_SCOPE)
endfunction()
