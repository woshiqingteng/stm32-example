add_baremetal_app(42_fatfs
    SOURCE main.c
    BSP     sdio
    LIB     fatfs_nand
)
