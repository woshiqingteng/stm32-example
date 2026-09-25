# Board support package. Every board driver is listed explicitly (no GLOB) and
# compiled into the bsp library; the linker keeps only the objects an image
# actually references. Applications link bsp (and optionally LIB components).
set(BSP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/24cxx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ap3216c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/atim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/btim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/can.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/delay.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dht11.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ds18b20.c
    ${CMAKE_CURRENT_SOURCE_DIR}/es8388.c
    ${CMAKE_CURRENT_SOURCE_DIR}/exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gtim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/iap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/iic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/key.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lcd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/norflash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nrf24l01.c
    ${CMAKE_CURRENT_SOURCE_DIR}/oled.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ov5640.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pwmdac.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/qmi8658a.c
    ${CMAKE_CURRENT_SOURCE_DIR}/remote.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rng.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rs485.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sai.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sdram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/spa06.c
    ${CMAKE_CURRENT_SOURCE_DIR}/spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stmflash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sys.c
    ${CMAKE_CURRENT_SOURCE_DIR}/timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/touch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tpad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/wdg.c)

add_library(bsp STATIC ${BSP_SRC})

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp PUBLIC hal_all)

# When the FreeRTOS module is enabled, the board layer uses the RTOS timing
# services (delay/tick in delay.c). The OS selection is driven here, not by the
# application, so bsp stays self-contained.
if(MODULE_FREERTOS_ENABLE)
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
endif()
