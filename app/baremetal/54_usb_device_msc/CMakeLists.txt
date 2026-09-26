add_baremetal_app(54_usb_device_msc
    SOURCE main.c
    BSP     sdio
    LIB     stm32_usb_device_msc nand_storage
)

