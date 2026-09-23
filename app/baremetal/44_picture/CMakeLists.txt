# 44_picture: BMP/JPEG/GIF viewer on the RGB panel. Needs the FatFs core/port
# (SD card), the PICTURE middleware (which pulls in TEXT and LibJPEG) and the
# text renderer.
add_baremetal_app(44_picture
    SOURCES main.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}                 ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_PICTURE}
    DEPS    fatfs_core fatfs_port
)

