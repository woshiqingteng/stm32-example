add_baremetal_app(43_font
    SOURCE main.c
    BSP     lcd sdio sdram
    LIB     malloc text fatfs
)

