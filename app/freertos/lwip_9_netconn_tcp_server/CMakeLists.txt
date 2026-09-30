add_freertos_app(lwip_9_netconn_tcp_server
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip text)
