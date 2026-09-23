# 53_iap_app: IAP application image linked at 0x08010000 (jumped to by the
# 53_iap bootloader). The linker script lives in this app's own directory and
# is selected through the MCU_LD_SCRIPT target property; the flash address is
# overridden so `--target 53_iap_app_flash` programs the image correctly.
set(MCU_FLASH_ADDR 0x08010000)

add_baremetal_app(53_iap_app
    SOURCES main.c
    BSP     ${BSP_SRC_LED} ${BSP_SRC_KEY} ${BSP_SRC_USART}
)

set_target_properties(app_baremetal_53_iap_app PROPERTIES
    MCU_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_iap_app.ld)
