# 34_i2c_magnet: ST480MC 3-axis magnetometer. The heading uses atan(), so libm
# is linked directly from this app.
add_baremetal_app(34_i2c_magnet
    SOURCE main.c
    BSP     st480mc
)

target_link_libraries(app_baremetal_34_i2c_magnet PRIVATE m)
