add_baremetal_app(24_iic
    SOURCES main.c
        BSP     ${BSP_SRC_IIC} ${BSP_SRC_24CXX} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
