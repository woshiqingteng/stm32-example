add_baremetal_app(47_sai_record
    SOURCE main.c recorder.c
    BSP     codec sai sdio sdram
    LIB     malloc audio
)

