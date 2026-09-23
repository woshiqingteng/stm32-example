add_baremetal_app(23_pwm_dac
    SOURCES main.c
    BSP     ${BSP_SRC_DAC} ${BSP_SRC_PWMDAC}
)
