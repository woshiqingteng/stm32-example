add_freertos_app(lwip_18_onenet_mqtt
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
