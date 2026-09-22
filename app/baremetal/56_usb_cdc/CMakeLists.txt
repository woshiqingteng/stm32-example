# 56_usb_cdc: USB device CDC virtual COM port. Needs the ST USB device stack
# and the board USB port (PCD + conf).
add_baremetal_app(56_usb_cdc
    SOURCES main.c
        BSP     ${BSP_SRC_LCD} ${BSP_SRC_SDRAM}
)

target_link_libraries(app_baremetal_56_usb_cdc PRIVATE usb_device usb_port)
