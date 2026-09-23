# 47_recorder: WAV recorder. The recorder state machine (recorder.c) is
# app-local; the AUDIO middleware provides the WAV playback device.
add_baremetal_app(47_recorder
    SOURCES main.c recorder.c
    BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SDRAM}
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_AUDIO}
)

