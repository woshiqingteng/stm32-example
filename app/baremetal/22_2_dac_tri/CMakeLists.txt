add_baremetal_app(22_2_dac_tri
    SOURCES main.c
        BSP     ${BSP_SRC_DAC} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
