add_freertos_app(lwip_8_netconn_tcp_client
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip text)
