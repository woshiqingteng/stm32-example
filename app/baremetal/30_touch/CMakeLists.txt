add_baremetal_app(30_touch
    SOURCES main.c
        BSP     ${BSP_SRC_IIC} ${BSP_SRC_TOUCH} ${BSP_SRC_24CXX} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
