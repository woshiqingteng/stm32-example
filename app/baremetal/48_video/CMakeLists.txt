add_baremetal_app(48_video
    SOURCE  main.c
    BSP     lcd sdio sdram
    LIB     malloc text mjpeg
)

