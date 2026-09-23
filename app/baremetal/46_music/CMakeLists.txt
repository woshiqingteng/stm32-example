# 46_music: WAV player. The playlist/navigation (audioplay.c) is app-local;
# the AUDIO middleware provides the self-contained WAV playback device.
add_baremetal_app(46_music
    SOURCES main.c audioplay.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SDRAM}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_AUDIO}
)

