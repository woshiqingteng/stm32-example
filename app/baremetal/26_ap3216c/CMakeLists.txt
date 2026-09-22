add_baremetal_app(26_ap3216c
    SOURCES main.c
        BSP     ${BSP_SRC_IIC} ${BSP_SRC_AP3216C} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
