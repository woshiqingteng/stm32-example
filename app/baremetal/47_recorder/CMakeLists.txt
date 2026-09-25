# 47_recorder: WAV recorder. The recorder state machine (recorder.c) is
# app-local; the AUDIO middleware provides the WAV playback device.
add_baremetal_app(47_recorder
    SOURCES main.c recorder.c
    LIB     MALLOC AUDIO
    BSP     es8388 sai sdio sdram
)

