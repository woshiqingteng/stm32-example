add_freertos_app(lwip_13_ntp
    SOURCE  main.c lwip_demo_ui.c
    BSP     eth key led lcd sdram rtc
    LIB     lwip text)
