add_baremetal_app(39_malloc
    SOURCES main.c
    BSP     ${BSP_SRC_SDRAM}
    LIB     ${LIB_SRC_MALLOC}
)
