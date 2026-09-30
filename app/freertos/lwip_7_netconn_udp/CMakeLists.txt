add_freertos_app(lwip_7_netconn_udp
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip text)
