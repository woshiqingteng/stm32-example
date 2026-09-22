add_baremetal_app(31_remote
    SOURCES main.c
        BSP     ${BSP_SRC_REMOTE} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
