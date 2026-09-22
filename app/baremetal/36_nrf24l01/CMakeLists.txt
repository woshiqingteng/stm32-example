add_baremetal_app(36_nrf24l01
    SOURCES main.c
        BSP     ${BSP_SRC_SPI} ${BSP_SRC_NRF24L01} ${BSP_SRC_SDRAM} ${BSP_SRC_LCD} ${BSP_SRC_KEY}
)
