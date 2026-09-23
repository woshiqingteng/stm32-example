# 46_music: WAV player. The playlist/navigation (audioplay.c) is app-local;
# the AUDIO middleware provides the self-contained WAV playback device.
add_baremetal_app(46_music
    SOURCES main.c audioplay.c
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_AUDIO}
)

