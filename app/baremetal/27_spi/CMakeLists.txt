add_baremetal_app(27_spi
    SOURCES main.c
        BSP     ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
