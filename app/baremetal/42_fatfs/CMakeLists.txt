add_baremetal_app(42_fatfs
    SOURCES main.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH} ${BSP_SRC_LCD}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_FATFS}
)
