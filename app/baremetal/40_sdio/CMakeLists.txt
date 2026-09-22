add_baremetal_app(40_sdio
    SOURCES main.c
        BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SRAM} ${BSP_SRC_LCD}
        LIB     ${LIB_SRC_MALLOC}
)
