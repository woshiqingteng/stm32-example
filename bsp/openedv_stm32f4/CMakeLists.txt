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
target_link_libraries(bsp_usart PUBLIC drv_uart)
bsp_library(bsp_core
    bsp.c)
target_link_libraries(bsp_core PUBLIC bsp_sys bsp_delay bsp_led bsp_key bsp_usart)

# time
bsp_library(bsp_btim
    btim.c)
target_link_libraries(bsp_btim PUBLIC drv_tim)
bsp_library(bsp_gtim
    gtim.c
    timer.c)
target_link_libraries(bsp_gtim PUBLIC drv_tim)
bsp_library(bsp_atim
    atim.c)
target_link_libraries(bsp_atim PUBLIC drv_tim)
bsp_library(bsp_ir
    ir.c)
target_link_libraries(bsp_ir PUBLIC drv_tim)
bsp_library(bsp_rtc
    rtc.c)
target_link_libraries(bsp_rtc PUBLIC drv_rtc)
bsp_library(bsp_wdg
    wdg.c)
target_link_libraries(bsp_wdg PUBLIC drv_iwdg drv_wwdg)

# comm
bsp_library(bsp_i2c
    i2c.c)
bsp_library(bsp_spi
    spi.c)
target_link_libraries(bsp_spi PUBLIC drv_spi)
bsp_library(bsp_pcf8574
    pcf8574.c)
target_link_libraries(bsp_pcf8574 PUBLIC bsp_i2c)
bsp_library(bsp_24cxx
    24cxx.c)
target_link_libraries(bsp_24cxx PUBLIC bsp_i2c)
bsp_library(bsp_can
    can.c)
target_link_libraries(bsp_can PUBLIC drv_can)
bsp_library(bsp_dcmi
    dcmi.c)
target_link_libraries(bsp_dcmi PUBLIC drv_dcmi bsp_ov5640)
bsp_library(bsp_ov5640
    ov5640.c)
target_link_libraries(bsp_ov5640 PUBLIC bsp_pcf8574)
bsp_library(bsp_es8388
    es8388.c)
target_link_libraries(bsp_es8388 PUBLIC bsp_i2c)
bsp_library(bsp_sai
    sai.c)
target_link_libraries(bsp_sai PUBLIC drv_sai)
bsp_library(bsp_rs485
    rs485.c)
target_link_libraries(bsp_rs485 PUBLIC bsp_pcf8574 drv_uart)
bsp_library(bsp_nrf24l01
    nrf24l01.c)
target_link_libraries(bsp_nrf24l01 PUBLIC bsp_spi)

# analog
bsp_library(bsp_adc
    adc.c)
target_link_libraries(bsp_adc PUBLIC drv_adc)
bsp_library(bsp_dac
    dac.c)
target_link_libraries(bsp_dac PUBLIC drv_dac drv_tim)
bsp_library(bsp_pwmdac
    pwmdac.c)
target_link_libraries(bsp_pwmdac PUBLIC bsp_dac drv_tim)

# memory
bsp_library(bsp_stmflash
    stmflash.c)
target_link_libraries(bsp_stmflash PUBLIC drv_flash)
bsp_library(bsp_sram
    sram.c)
target_link_libraries(bsp_sram PUBLIC drv_fmc)
bsp_library(bsp_sdram
    sdram.c)
target_link_libraries(bsp_sdram PUBLIC drv_fmc)
bsp_library(bsp_nand
    nand.c)
target_link_libraries(bsp_nand PUBLIC drv_fmc)
bsp_library(bsp_norflash
    norflash.c)
target_link_libraries(bsp_norflash PUBLIC bsp_spi)
bsp_library(bsp_sdio
    sdio.c)
target_link_libraries(bsp_sdio PUBLIC drv_sd)

# display
bsp_library(bsp_ltdc
    ltdc.c)
target_link_libraries(bsp_ltdc PUBLIC drv_ltdc drv_dma2d)
bsp_library(bsp_lcd
    lcd.c)
target_link_libraries(bsp_lcd PUBLIC bsp_ltdc)
bsp_library(bsp_oled
    oled.c)
bsp_library(bsp_touch
    touch.c)
bsp_library(bsp_tpad
    tpad.c)
target_link_libraries(bsp_tpad PUBLIC drv_tim)

# crypto
bsp_library(bsp_rng
    rng.c)
target_link_libraries(bsp_rng PUBLIC drv_rng)

# sensor
bsp_library(bsp_ap3216c
    ap3216c.c)
target_link_libraries(bsp_ap3216c PUBLIC bsp_i2c)
bsp_library(bsp_qmi8658a
    qmi8658a.c)
target_link_libraries(bsp_qmi8658a PUBLIC bsp_i2c)
bsp_library(bsp_spa06
    spa06.c)
target_link_libraries(bsp_spa06 PUBLIC bsp_i2c)
bsp_library(bsp_ds18b20
    ds18b20.c)
bsp_library(bsp_dht11
    dht11.c)

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
    bsp_btim bsp_gtim bsp_atim bsp_ir bsp_rtc bsp_wdg
    bsp_i2c bsp_spi bsp_pcf8574 bsp_24cxx
    bsp_can bsp_dcmi bsp_ov5640 bsp_es8388 bsp_sai bsp_rs485 bsp_nrf24l01
    bsp_adc bsp_dac bsp_pwmdac
    bsp_stmflash bsp_sram bsp_sdram bsp_nand bsp_norflash bsp_sdio
    bsp_ltdc bsp_lcd bsp_oled bsp_touch bsp_tpad
    bsp_rng
    bsp_ap3216c bsp_qmi8658a bsp_spa06 bsp_ds18b20 bsp_dht11
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
