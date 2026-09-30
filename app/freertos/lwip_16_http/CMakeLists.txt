add_freertos_app(lwip_16_http
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
