add_baremetal_app(41_nand
    SOURCES main.c
        BSP     ${BSP_SRC_NAND} ${BSP_SRC_LCD}
        LIB     ${LIB_SRC_MALLOC}
)
