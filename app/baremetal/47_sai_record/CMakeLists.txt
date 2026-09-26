# 47_sai_record: WAV recorder. The recorder state machine (recorder.c) is
# app-local; the AUDIO middleware provides the WAV playback device.
add_baremetal_app(47_sai_record
    SOURCES main.c recorder.c
    BSP     codec sai sdio sdram
    LIB     malloc audio
)

