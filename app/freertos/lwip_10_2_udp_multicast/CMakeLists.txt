add_freertos_app(lwip_10_2_udp_multicast
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
