# 44_picture: BMP/JPEG/GIF viewer on the RGB panel. Needs the FatFs core/port
# (SD card), the PICTURE middleware (which pulls in TEXT and LibJPEG) and the
# text renderer.
add_baremetal_app(44_picture
    SOURCES main.c
    LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT} ${LIB_SRC_PICTURE}
)

