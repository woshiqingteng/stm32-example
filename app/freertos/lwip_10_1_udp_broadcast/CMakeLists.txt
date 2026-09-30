add_freertos_app(lwip_10_1_udp_broadcast
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip text)
