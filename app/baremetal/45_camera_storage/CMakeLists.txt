add_baremetal_app(45_camera_storage
    SOURCE main.c
    BSP     dcmi lcd ov5640 io_expand sdio sdram gtim
    LIB     malloc text picture cam_jpeg
)

