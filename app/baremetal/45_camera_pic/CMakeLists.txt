# 45_camera_pic: OV5640 live view + JPEG capture/display. Needs the camera BSP
# (DCMI/OV5640/PCF8574/TIMER), the FatFs core/port (SD card) and the PICTURE
# middleware (TEXT + LibJPEG) for JPEG encode/decode.
add_baremetal_app(45_camera_pic
    SOURCES main.c
    BSP     ${BSP_SRC_DCMI} ${BSP_SRC_OV5640} ${BSP_SRC_PCF8574} ${BSP_SRC_IIC}                 ${BSP_SRC_TIMER} ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}                 ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_PICTURE}
    DEPS    fatfs_core fatfs_port
)

