# Board support package component registry.
#
# Each component declares its sources (BSP_SRC_<name>); the shared core sources
# live in BSP_SRC_COMMON. The DRV (HAL) is linked wholesale and the linker pulls
# only the objects an image references, so apps list just the BSP/LIB components
# they use and never mention HAL.

set(BSP_SRC_BSP    ${CMAKE_CURRENT_SOURCE_DIR}/bsp.c CACHE INTERNAL "")
set(BSP_SRC_SYS    ${CMAKE_CURRENT_SOURCE_DIR}/sys.c CACHE INTERNAL "")
set(BSP_SRC_DELAY  ${CMAKE_CURRENT_SOURCE_DIR}/delay.c CACHE INTERNAL "")
set(BSP_SRC_USART  ${CMAKE_CURRENT_SOURCE_DIR}/usart.c CACHE INTERNAL "")
set(BSP_SRC_LED    ${CMAKE_CURRENT_SOURCE_DIR}/led.c CACHE INTERNAL "")
set(BSP_SRC_KEY    ${CMAKE_CURRENT_SOURCE_DIR}/key.c CACHE INTERNAL "")

set(BSP_SRC_EXTI   ${CMAKE_CURRENT_SOURCE_DIR}/exti.c CACHE INTERNAL "")
set(BSP_SRC_ADC    ${CMAKE_CURRENT_SOURCE_DIR}/adc.c CACHE INTERNAL "")
set(BSP_SRC_DAC    ${CMAKE_CURRENT_SOURCE_DIR}/dac.c CACHE INTERNAL "")
set(BSP_SRC_PWMDAC ${CMAKE_CURRENT_SOURCE_DIR}/pwmdac.c CACHE INTERNAL "")
set(BSP_SRC_STMFLASH ${CMAKE_CURRENT_SOURCE_DIR}/stmflash.c CACHE INTERNAL "")
set(BSP_SRC_WDG    ${CMAKE_CURRENT_SOURCE_DIR}/wdg.c CACHE INTERNAL "")
set(BSP_SRC_TIM    ${CMAKE_CURRENT_SOURCE_DIR}/btim.c
                   ${CMAKE_CURRENT_SOURCE_DIR}/gtim.c
                   ${CMAKE_CURRENT_SOURCE_DIR}/atim.c CACHE INTERNAL "")
set(BSP_SRC_TPAD   ${CMAKE_CURRENT_SOURCE_DIR}/tpad.c CACHE INTERNAL "")
set(BSP_SRC_SDRAM  ${CMAKE_CURRENT_SOURCE_DIR}/sdram.c CACHE INTERNAL "")
set(BSP_SRC_LCD    ${CMAKE_CURRENT_SOURCE_DIR}/lcd.c
                   ${CMAKE_CURRENT_SOURCE_DIR}/ltdc.c CACHE INTERNAL "")
set(BSP_SRC_OLED   ${CMAKE_CURRENT_SOURCE_DIR}/oled.c CACHE INTERNAL "")
set(BSP_SRC_RTC    ${CMAKE_CURRENT_SOURCE_DIR}/rtc.c CACHE INTERNAL "")
set(BSP_SRC_RNG    ${CMAKE_CURRENT_SOURCE_DIR}/rng.c CACHE INTERNAL "")
set(BSP_SRC_PWR    ${CMAKE_CURRENT_SOURCE_DIR}/pwr.c CACHE INTERNAL "")
set(BSP_SRC_IAP    ${CMAKE_CURRENT_SOURCE_DIR}/iap.c CACHE INTERNAL "")
set(BSP_SRC_IIC    ${CMAKE_CURRENT_SOURCE_DIR}/iic.c CACHE INTERNAL "")
set(BSP_SRC_24CXX  ${CMAKE_CURRENT_SOURCE_DIR}/24cxx.c CACHE INTERNAL "")
set(BSP_SRC_PCF8574 ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574.c CACHE INTERNAL "")
set(BSP_SRC_AP3216C ${CMAKE_CURRENT_SOURCE_DIR}/ap3216c.c CACHE INTERNAL "")
set(BSP_SRC_TOUCH  ${CMAKE_CURRENT_SOURCE_DIR}/touch.c CACHE INTERNAL "")
set(BSP_SRC_SPA06  ${CMAKE_CURRENT_SOURCE_DIR}/spa06.c CACHE INTERNAL "")
set(BSP_SRC_QMI8658A ${CMAKE_CURRENT_SOURCE_DIR}/qmi8658a.c CACHE INTERNAL "")
set(BSP_SRC_SPI    ${CMAKE_CURRENT_SOURCE_DIR}/spi.c CACHE INTERNAL "")
set(BSP_SRC_NORFLASH ${CMAKE_CURRENT_SOURCE_DIR}/norflash.c CACHE INTERNAL "")
set(BSP_SRC_RS485  ${CMAKE_CURRENT_SOURCE_DIR}/rs485.c CACHE INTERNAL "")
set(BSP_SRC_CAN    ${CMAKE_CURRENT_SOURCE_DIR}/can.c CACHE INTERNAL "")
set(BSP_SRC_REMOTE ${CMAKE_CURRENT_SOURCE_DIR}/remote.c CACHE INTERNAL "")
set(BSP_SRC_DS18B20 ${CMAKE_CURRENT_SOURCE_DIR}/ds18b20.c CACHE INTERNAL "")
set(BSP_SRC_DHT11  ${CMAKE_CURRENT_SOURCE_DIR}/dht11.c CACHE INTERNAL "")
set(BSP_SRC_NRF24L01 ${CMAKE_CURRENT_SOURCE_DIR}/nrf24l01.c CACHE INTERNAL "")
set(BSP_SRC_SDIO   ${CMAKE_CURRENT_SOURCE_DIR}/sdio.c CACHE INTERNAL "")
set(BSP_SRC_SRAM   ${CMAKE_CURRENT_SOURCE_DIR}/sram.c CACHE INTERNAL "")
set(BSP_SRC_NAND   ${CMAKE_CURRENT_SOURCE_DIR}/nand.c CACHE INTERNAL "")
set(BSP_SRC_DCMI   ${CMAKE_CURRENT_SOURCE_DIR}/dcmi.c CACHE INTERNAL "")
set(BSP_SRC_OV5640 ${CMAKE_CURRENT_SOURCE_DIR}/ov5640.c CACHE INTERNAL "")
set(BSP_SRC_TIMER  ${CMAKE_CURRENT_SOURCE_DIR}/timer.c CACHE INTERNAL "")
set(BSP_SRC_ES8388 ${CMAKE_CURRENT_SOURCE_DIR}/es8388.c CACHE INTERNAL "")
set(BSP_SRC_SAI    ${CMAKE_CURRENT_SOURCE_DIR}/sai.c CACHE INTERNAL "")

# Shared core BSP sources, always compiled into the bsp target.
set(BSP_SRC_COMMON
    ${BSP_SRC_BSP} ${BSP_SRC_SYS} ${BSP_SRC_DELAY}
    ${BSP_SRC_USART} ${BSP_SRC_LED} ${BSP_SRC_KEY} CACHE INTERNAL "")

set(BSP_COMPONENTS EXTI ADC WDG TIM TPAD SDRAM LCD OLED RTC RNG PWR IAP
    DAC PWMDAC STMFLASH
    IIC 24CXX PCF8574 AP3216C TOUCH SPA06 QMI8658A
    SPI NORFLASH RS485 CAN REMOTE DS18B20 DHT11 NRF24L01
    SDIO SRAM NAND
    DCMI OV5640 TIMER
    ES8388 SAI
    CACHE INTERNAL "")

# Component -> component dependencies: components that sit on top of another
# BSP component (e.g. sensors on the IIC bus). Expanded transitively by
# bsp_expand so apps only list the top-level component they use.
set(BSP_SRC_24CXX_DEPS    ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_PCF8574_DEPS  ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_AP3216C_DEPS  ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_TOUCH_DEPS    ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_SPA06_DEPS    ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_QMI8658A_DEPS ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_ES8388_DEPS   ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_OV5640_DEPS   ${BSP_SRC_IIC} CACHE INTERNAL "")
set(BSP_SRC_NORFLASH_DEPS ${BSP_SRC_SPI} CACHE INTERNAL "")
set(BSP_SRC_NRF24L01_DEPS ${BSP_SRC_SPI} CACHE INTERNAL "")
set(BSP_SRC_RS485_DEPS    ${BSP_SRC_PCF8574} CACHE INTERNAL "")

# bsp_expand(<out-var> <bsp sources...>): transitively add the sources of every
# component whose sources are (fully) present in the input list.
function(bsp_expand out)
    set(_acc ${ARGN})
    set(_changed TRUE)
    while(_changed)
        set(_changed FALSE)
        foreach(_comp ${BSP_COMPONENTS})
            set(_match TRUE)
            foreach(_f ${BSP_SRC_${_comp}})
                if(NOT (_f IN_LIST _acc))
                    set(_match FALSE)
                endif()
            endforeach()
            if(_match)
                foreach(_d ${BSP_SRC_${_comp}_DEPS})
                    if(NOT (_d IN_LIST _acc))
                        list(APPEND _acc ${_d})
                        set(_changed TRUE)
                    endif()
                endforeach()
            endif()
        endforeach()
    endwhile()
    list(REMOVE_DUPLICATES _acc)
    set(${out} ${_acc} PARENT_SCOPE)
endfunction()

# bsp_resolve_hal removed: the DRV (HAL) is linked wholesale and the linker
# keeps only the objects referenced by the image.

add_library(bsp STATIC)
target_sources(bsp PRIVATE ${BSP_SRC_COMMON})

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp PUBLIC stm32f4xx_hal ${TARGET_LIB})
