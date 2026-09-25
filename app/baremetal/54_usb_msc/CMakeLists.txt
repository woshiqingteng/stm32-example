# 54_usb_msc: USB device Mass Storage (SD card). Needs the SDIO BSP, the ST USB
# device stack and the board USB port (PCD + conf + MSC storage interface).
add_baremetal_app(54_usb_msc
    SOURCES main.c
    BSP     sdio
    LIB     usb_device_msc
)

