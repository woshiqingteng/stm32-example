# 57_usb_host_msc: USB host Mass Storage. Enumerates a USB flash drive and
# mounts it through FatFs as logical drive "2:". Needs the ST USB host stack,
# the board USB port (HCD + conf) and the FatFs core/port.
add_baremetal_app(57_usb_host_msc
    SOURCES main.c
    BSP     ${BSP_SRC_IIC} ${BSP_SRC_PCF8574} ${BSP_SRC_LCD} ${BSP_SRC_SDRAM}
            ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}
    LIB     ${LIB_SRC_FATFS} ${LIB_SRC_USB_HOST}
)

# Route FatFs drive 2 to the USB MSC glue in port/usb_port/usbh_diskio.c.
target_compile_definitions(fatfs_port PRIVATE FATFS_USB_MSC=1)
target_include_directories(fatfs_port PRIVATE ${CMAKE_SOURCE_DIR}/port/usb_port)
