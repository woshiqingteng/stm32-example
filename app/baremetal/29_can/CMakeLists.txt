add_baremetal_app(29_can
    SOURCES main.c
    BSP     ${BSP_SRC_CAN} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
