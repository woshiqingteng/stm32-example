add_baremetal_app(35_i2c_imu
    SOURCE main.c imu.c
    BSP     qmi8658a
    LIB     dsp
)
