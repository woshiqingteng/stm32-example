add_freertos_app(54_usb_device_msc
    SOURCE  main.c
    BSP     sdio nor ftl nand
    LIB     cherryusb_device)
