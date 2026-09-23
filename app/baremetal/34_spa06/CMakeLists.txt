add_baremetal_app(34_spa06
    SOURCES main.c
    BSP     ${BSP_SRC_IIC} ${BSP_SRC_SPA06} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
