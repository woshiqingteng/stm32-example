# 56_usb_device_cdc: USB device CDC virtual COM port. Needs the ST USB device stack
# and the board USB port (PCD + conf).
add_baremetal_app(56_usb_device_cdc
    SOURCE main.c
    LIB     stm32_usb_device_cdc
)

