# 43_text: GBK text display on the RGB panel. Needs the FatFs core/port (SD
# card) plus the TEXT middleware and its NOR-flash font store.
add_baremetal_app(43_text
    SOURCES main.c
        BSP     ${BSP_SRC_SDIO} ${BSP_SRC_SPI} ${BSP_SRC_NORFLASH}
                ${BSP_SRC_SDRAM} ${BSP_SRC_LCD}
        LIB     ${LIB_SRC_MALLOC} ${LIB_SRC_TEXT}
)

target_link_libraries(app_baremetal_43_text PRIVATE fatfs_core fatfs_port)
