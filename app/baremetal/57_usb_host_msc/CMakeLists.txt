# 57_usb_host_msc: USB host Mass Storage. Enumerates a USB flash drive and
# mounts it through FatFs as logical drive "2:". Needs the ST USB host stack,
# the board USB port (HCD + conf) and the FatFs core/port.
add_baremetal_app(57_usb_host_msc
    SOURCES main.c
    LIB     FATFS USB_HOST
)

# Route FatFs drive 2 to the USB MSC glue in port/usb_port/usbh_diskio.c.
target_compile_definitions(fatfs_port PRIVATE FATFS_USB_MSC=1)
target_include_directories(fatfs_port PRIVATE ${CMAKE_SOURCE_DIR}/port/usb_port)
