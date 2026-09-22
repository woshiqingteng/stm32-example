add_baremetal_app(42_fatfs
    SOURCES main.c
        BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH} ${BSP_SRC_LCD}
        LIB     ${LIB_SRC_MALLOC}
)

# FatFs core plus the physical drive glue are provided by the port layer.
# Order matters: ff.c references diskio symbols, so the core archive must come
# before the port archive on the static link line.
target_link_libraries(app_baremetal_42_fatfs PRIVATE fatfs_core fatfs_port)
