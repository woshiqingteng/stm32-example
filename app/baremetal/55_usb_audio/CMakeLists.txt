# 55_usb_audio: USB device Audio (ES8388 speaker + microphone monitor). Needs
# the ES8388/SAI/IIC BSP, the ST USB device stack and the board USB port.
add_baremetal_app(55_usb_audio
    SOURCES main.c
    LIB     ${LIB_SRC_USB_DEVICE}
)

