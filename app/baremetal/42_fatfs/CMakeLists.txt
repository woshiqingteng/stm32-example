add_baremetal_app(42_fatfs
    SOURCES main.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_NORFLASH}
    LIB     ${LIB_SRC_FATFS}
)
