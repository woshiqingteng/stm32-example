add_baremetal_app(25_io_extend
    SOURCES main.c
        BSP     ${BSP_SRC_IIC} ${BSP_SRC_PCF8574} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
