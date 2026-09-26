add_baremetal_app(38_camera_stream
    SOURCES main.c
    BSP     dcmi lcd ov5640 sdram gtim btim
)
