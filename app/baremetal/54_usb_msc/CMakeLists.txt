# 54_usb_msc: USB device Mass Storage (SD card). Needs the SDIO BSP, the ST USB
# device stack and the board USB port (PCD + conf + MSC storage interface).
add_baremetal_app(54_usb_msc
    SOURCES main.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_LCD} ${BSP_SRC_SDRAM}
    DEPS    usb_device usb_port
)

