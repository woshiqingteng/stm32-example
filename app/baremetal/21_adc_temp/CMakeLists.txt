add_baremetal_app(21_adc_temp
    SOURCES main.c
        BSP     ${BSP_SRC_ADC} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
