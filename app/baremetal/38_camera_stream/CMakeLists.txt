add_baremetal_app(38_camera_stream
    SOURCE main.c
    BSP     dcmi lcd ov5640 sdram gtim btim
    LIB     cam_jpeg
)
