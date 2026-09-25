# 58_usb_host_hid: USB host HID (mouse / keyboard). Needs the ST USB host
# stack and the board USB port (HCD + conf; USB VBUS through the PCF8574).
add_baremetal_app(58_usb_host_hid
    SOURCES main.c
    BSP     pcf8574
    LIB     stm32_usb_host_hid
)

