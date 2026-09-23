# 46_music: WAV/MP3 player. Needs the FatFs core/port (SD card), the AUDIO
# middleware (which pulls in ES8388/SAI/IIC and the Helix MP3 decoder) and the
# text renderer.
add_baremetal_app(46_music
    SOURCES main.c
    BSP     ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_SDRAM}                 ${BSP_SRC_LCD} ${BSP_SRC_KEY}                 ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_AUDIO}
    DEPS    fatfs_core fatfs_port
)

