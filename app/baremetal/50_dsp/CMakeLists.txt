add_baremetal_app(50_dsp
    SOURCES main.c
        BSP     ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)

target_link_libraries(app_baremetal_50_dsp PRIVATE cmsis_dsp m)
