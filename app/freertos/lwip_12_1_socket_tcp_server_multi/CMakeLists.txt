add_freertos_app(lwip_12_1_socket_tcp_server_multi
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
