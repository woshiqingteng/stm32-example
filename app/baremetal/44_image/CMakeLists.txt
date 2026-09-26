add_baremetal_app(44_image
    SOURCE  main.c
    BSP     lcd sdio sdram
    LIB     malloc text picture
)

