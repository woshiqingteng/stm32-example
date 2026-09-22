add_baremetal_app(49_fpu
    SOURCES main.c
        BSP     ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
