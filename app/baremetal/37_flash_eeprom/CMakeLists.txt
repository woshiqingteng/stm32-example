add_baremetal_app(37_flash_eeprom
    SOURCES main.c
    BSP     ${BSP_SRC_STMFLASH} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
