# 46_music: WAV player. The playlist/navigation (audioplay.c) is app-local;
# the AUDIO middleware provides the self-contained WAV playback device.
add_baremetal_app(46_music
    SOURCES main.c audioplay.c
    BSP     ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_SDRAM}                 ${BSP_SRC_LCD} ${BSP_SRC_KEY}                 ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_AUDIO}
)

