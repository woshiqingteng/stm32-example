add_baremetal_app(50_dsp
    SOURCES main.c
    BSP     ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
    LIB     ${LIB_SRC_DSP}
)

