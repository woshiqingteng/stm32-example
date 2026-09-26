set(MCU_FLASH_ADDR 0x08010000)

add_baremetal_app(53_iap_app
    SOURCE main.c
)

set_target_properties(app_baremetal_53_iap_app PROPERTIES
    MCU_LD_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_iap_app.ld)
