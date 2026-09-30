add_freertos_app(lwip_6_freertos
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
