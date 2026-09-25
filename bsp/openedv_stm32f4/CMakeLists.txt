# Board support package: one STATIC library per board driver (the "small
# class"), grouped by big category in comment blocks. Everything is
# EXCLUDE_FROM_ALL: a driver is compiled only when some built target depends on
# it. bsp_core is the always-linked set. Applications declare the extra drivers
# via the BSP keyword (bsp_resolve below); HAL is pulled in implicitly.

function(bsp_leaf name)
    add_library(${name} STATIC EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PUBLIC drv_core)
endfunction()

# ===== core =================================================================
bsp_leaf(bsp_init  bsp.c)
bsp_leaf(bsp_sys   sys.c)
bsp_leaf(bsp_delay delay.c)
bsp_leaf(bsp_led   led.c)
bsp_leaf(bsp_key   key.c)
bsp_leaf(bsp_usart usart.c)
target_link_libraries(bsp_usart PUBLIC drv_uart)
target_link_libraries(bsp_init PUBLIC bsp_sys bsp_delay bsp_led bsp_key bsp_usart)

# The board layer uses the RTOS timing services (delay/tick) when FreeRTOS runs.
if(MODULE_FREERTOS_ENABLE)
    target_compile_definitions(bsp_delay PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp_delay PUBLIC freertos)
endif()

# Always-linked set. HAL is pulled in through these leaves.
add_library(bsp_core INTERFACE)
target_link_libraries(bsp_core INTERFACE
    bsp_init bsp_sys bsp_delay bsp_led bsp_key bsp_usart)

# ===== time =================================================================
bsp_leaf(bsp_btim btim.c)
bsp_leaf(bsp_gtim gtim.c timer.c)
bsp_leaf(bsp_atim atim.c)
bsp_leaf(bsp_ir   ir.c)
bsp_leaf(bsp_rtc  rtc.c)
bsp_leaf(bsp_wdg  wdg.c)
target_link_libraries(bsp_btim PUBLIC drv_tim)
target_link_libraries(bsp_gtim PUBLIC drv_tim)
target_link_libraries(bsp_atim PUBLIC drv_tim)
target_link_libraries(bsp_ir   PUBLIC drv_tim)
target_link_libraries(bsp_rtc  PUBLIC drv_rtc)
target_link_libraries(bsp_wdg  PUBLIC drv_iwdg drv_wwdg)

# ===== comm_basic ===========================================================
bsp_leaf(bsp_i2c     i2c.c)
bsp_leaf(bsp_spi     spi.c)
bsp_leaf(bsp_pcf8574 pcf8574.c)
bsp_leaf(bsp_24cxx   24cxx.c)
target_link_libraries(bsp_spi     PUBLIC drv_spi)
target_link_libraries(bsp_pcf8574 PUBLIC bsp_i2c)
target_link_libraries(bsp_24cxx   PUBLIC bsp_i2c)

# ===== comm_can / comm_dcmi / comm_audio / comm_misc ========================
bsp_leaf(bsp_can     can.c)
bsp_leaf(bsp_dcmi    dcmi.c)
bsp_leaf(bsp_ov5640  ov5640.c)
bsp_leaf(bsp_es8388  es8388.c)
bsp_leaf(bsp_sai     sai.c)
bsp_leaf(bsp_rs485   rs485.c)
bsp_leaf(bsp_nrf24l01 nrf24l01.c)
target_link_libraries(bsp_can       PUBLIC drv_can)
target_link_libraries(bsp_dcmi      PUBLIC drv_dcmi)
target_link_libraries(bsp_ov5640    PUBLIC bsp_pcf8574)
target_link_libraries(bsp_es8388    PUBLIC bsp_i2c)
target_link_libraries(bsp_sai       PUBLIC drv_sai)
target_link_libraries(bsp_rs485     PUBLIC bsp_pcf8574 drv_uart)
target_link_libraries(bsp_nrf24l01  PUBLIC bsp_spi)

# ===== analog ===============================================================
bsp_leaf(bsp_adc    adc.c)
bsp_leaf(bsp_dac    dac.c)
bsp_leaf(bsp_pwmdac pwmdac.c)
target_link_libraries(bsp_adc    PUBLIC drv_adc)
target_link_libraries(bsp_dac    PUBLIC drv_dac drv_tim)
target_link_libraries(bsp_pwmdac PUBLIC bsp_dac drv_tim)

# ===== memory ===============================================================
bsp_leaf(bsp_stmflash stmflash.c)
bsp_leaf(bsp_sram     sram.c)
bsp_leaf(bsp_sdram    sdram.c)
bsp_leaf(bsp_nand     nand.c)
bsp_leaf(bsp_norflash norflash.c)
bsp_leaf(bsp_sdio     sdio.c)
target_link_libraries(bsp_stmflash PUBLIC drv_flash)
target_link_libraries(bsp_sram     PUBLIC drv_fmc)
target_link_libraries(bsp_sdram    PUBLIC drv_fmc)
target_link_libraries(bsp_nand     PUBLIC drv_fmc)
target_link_libraries(bsp_norflash PUBLIC bsp_spi)
target_link_libraries(bsp_sdio     PUBLIC drv_sd)

# ===== display ==============================================================
bsp_leaf(bsp_ltdc  ltdc.c)
bsp_leaf(bsp_lcd   lcd.c)
bsp_leaf(bsp_oled  oled.c)
bsp_leaf(bsp_touch touch.c)
bsp_leaf(bsp_tpad  tpad.c)
target_link_libraries(bsp_ltdc PUBLIC drv_ltdc drv_dma2d)
target_link_libraries(bsp_lcd  PUBLIC bsp_ltdc)
target_link_libraries(bsp_tpad PUBLIC drv_tim)

# ===== crypto ===============================================================
bsp_leaf(bsp_rng rng.c)
target_link_libraries(bsp_rng PUBLIC drv_rng)

# ===== sensor ===============================================================
bsp_leaf(bsp_ap3216c  ap3216c.c)
bsp_leaf(bsp_qmi8658a qmi8658a.c)
bsp_leaf(bsp_spa06    spa06.c)
bsp_leaf(bsp_ds18b20  ds18b20.c)
bsp_leaf(bsp_dht11    dht11.c)
target_link_libraries(bsp_ap3216c  PUBLIC bsp_i2c)
target_link_libraries(bsp_qmi8658a PUBLIC bsp_i2c)
target_link_libraries(bsp_spa06    PUBLIC bsp_i2c)

# ===== misc =================================================================
bsp_leaf(bsp_exti exti.c)
bsp_leaf(bsp_pwr  pwr.c)
bsp_leaf(bsp_iap  iap.c)
target_link_libraries(bsp_exti PUBLIC bsp_key)
target_link_libraries(bsp_pwr  PUBLIC bsp_exti)
target_link_libraries(bsp_iap  PUBLIC bsp_usart drv_flash)

# ===== aggregates / resolver ================================================
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

# bsp_resolve(<out> [core] <leaf...> | ALL): map app BSP components to targets.
# "core" is always included.
function(bsp_resolve out)
    set(_r bsp_core)
    foreach(_n ${ARGN})
        if(_n STREQUAL "ALL")
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
