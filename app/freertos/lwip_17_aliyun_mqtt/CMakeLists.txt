add_freertos_app(lwip_17_aliyun_mqtt
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram
    LIB     lwip)
