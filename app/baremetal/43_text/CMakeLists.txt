# 43_text: GBK text display on the RGB panel. Needs the FatFs core/port (SD
# card) plus the TEXT middleware and its NOR-flash font store.
add_baremetal_app(43_text
    SOURCES main.c
    LIB     MALLOC TEXT FATFS
)

