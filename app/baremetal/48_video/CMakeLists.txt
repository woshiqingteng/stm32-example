# 48_video: MJPEG/AVI video player. Needs the FatFs core/port (SD card), the
# MJPEG middleware (LibJPEG + AVI parser + TIM7 frame pacing, self-contained
# ES8388/SAI audio) and the text renderer.
add_baremetal_app(48_video
    SOURCES main.c
    LIB     MALLOC TEXT MJPEG
)

