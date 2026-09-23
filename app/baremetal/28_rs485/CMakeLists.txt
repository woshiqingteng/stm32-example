add_baremetal_app(28_rs485
    SOURCES main.c
    BSP     ${BSP_SRC_RS485} ${BSP_SRC_PCF8574} ${BSP_SRC_IIC} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
