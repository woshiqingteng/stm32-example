add_baremetal_app(46_sai_audio
    SOURCE  main.c audioplay.c
    BSP     sdio sdram
    LIB     malloc audio
)

