set(SOURCES nand_storage.c)

add_library(lib_nand_storage STATIC EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(lib_nand_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_nand_storage PUBLIC lib_ftl fatfs_core_inc)
