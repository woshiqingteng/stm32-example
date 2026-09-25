# 46_music: WAV player. The playlist/navigation (audioplay.c) is app-local;
# the AUDIO middleware provides the self-contained WAV playback device.
add_baremetal_app(46_music
    SOURCES main.c audioplay.c
    BSP     sdio sdram
    LIB     malloc audio
)

