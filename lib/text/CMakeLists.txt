# ALIENTEK TEXT middleware: GBK text rendering over the RGB panel plus the
# NOR-flash GBK font store.
set(TEXT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/text.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts.c)

add_library(lib_text STATIC EXCLUDE_FROM_ALL ${TEXT_SRC})

target_include_directories(lib_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_text PUBLIC bsp lib_malloc fatfs_core fatfs_port)
