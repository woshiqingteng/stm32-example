# 48_video: MJPEG/AVI video player. Needs the FatFs core/port (SD card), the
# MJPEG middleware (LibJPEG + AVI parser + TIM7 frame pacing, self-contained
# ES8388/SAI audio) and the text renderer.
add_baremetal_app(48_video
    SOURCES main.c
    BSP     ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_SDRAM}                 ${BSP_SRC_LCD} ${BSP_SRC_KEY}                 ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_MJPEG}
)

