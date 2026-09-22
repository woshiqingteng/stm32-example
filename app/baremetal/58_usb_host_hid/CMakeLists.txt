# 58_usb_host_hid: USB host HID (mouse / keyboard). Needs the ST USB host
# stack and the board USB port (HCD + conf; USB VBUS through the PCF8574).
add_baremetal_app(58_usb_host_hid
    SOURCES main.c
        BSP     ${BSP_SRC_IIC} ${BSP_SRC_PCF8574} ${BSP_SRC_LCD} ${BSP_SRC_SDRAM}
)

target_link_libraries(app_baremetal_58_usb_host_hid PRIVATE usb_host usb_port)
