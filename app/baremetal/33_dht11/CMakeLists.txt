add_baremetal_app(33_dht11
    SOURCES main.c
    BSP     ${BSP_SRC_DHT11} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
)
