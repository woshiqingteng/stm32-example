# 46_sai_audio: WAV player. The playlist/navigation (audioplay.c) is app-local;
# the AUDIO middleware provides the self-contained WAV playback device.
add_baremetal_app(46_sai_audio
    SOURCE main.c audioplay.c
    BSP     sdio sdram
    LIB     malloc audio
)

