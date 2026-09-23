add_baremetal_app(38_camera
    SOURCES main.c
    BSP     ${BSP_SRC_DCMI} ${BSP_SRC_OV5640} ${BSP_SRC_PCF8574} ${BSP_SRC_IIC}                 ${BSP_SRC_TIMER} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
