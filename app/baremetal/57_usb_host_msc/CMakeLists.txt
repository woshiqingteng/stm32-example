# 57_usb_host_msc: USB host Mass Storage. Enumerates a USB flash drive and
# mounts it through FatFs as logical drive "2:". Needs the ST USB host stack,
# the board USB port (HCD + conf) and the FatFs USB-MSC port variant.
add_baremetal_app(57_usb_host_msc
    SOURCE main.c
    LIB     fatfs_stm32_usb_msc stm32_usb_host_msc
)
