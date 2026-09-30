add_freertos_app(lwip_14_sntp
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram rtc
    LIB     lwip text)
