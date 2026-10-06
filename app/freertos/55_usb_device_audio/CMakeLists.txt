add_freertos_app(55_usb_device_audio
    SOURCE  main.c
    BSP     codec sai
    LIB     cherryusb_device)
