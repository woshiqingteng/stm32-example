# 56_usb_cdc: USB device CDC virtual COM port. Needs the ST USB device stack
# and the board USB port (PCD + conf).
add_baremetal_app(56_usb_cdc
    SOURCES main.c
    BSP     ${BSP_SRC_LCD} ${BSP_SRC_SDRAM}
    LIB     ${LIB_SRC_USB_DEVICE}
)

