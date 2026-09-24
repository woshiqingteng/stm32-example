# 45_camera_pic: OV5640 live view + JPEG capture/display. Needs the camera BSP
# (DCMI/OV5640/PCF8574/TIMER), the FatFs core/port (SD card) and the PICTURE
# middleware (TEXT + LibJPEG) for JPEG encode/decode.
add_baremetal_app(45_camera_pic
    SOURCES main.c
    LIB     MALLOC TEXT PICTURE
)

