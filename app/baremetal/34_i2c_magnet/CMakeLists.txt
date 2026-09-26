add_baremetal_app(34_i2c_magnet
    SOURCE main.c
    BSP     st480mc
)

target_link_libraries(app_baremetal_34_i2c_magnet PRIVATE m)
