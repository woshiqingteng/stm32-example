add_baremetal_app(20_1_adc
    SOURCES main.c
    HAL     ${HAL_SRC_ADC} ${HAL_SRC_DMA}
    BSP     ${BSP_SRC_ADC}
)
