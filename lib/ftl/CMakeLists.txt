set(FTL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ftl.c)

add_library(lib_ftl STATIC EXCLUDE_FROM_ALL ${FTL_SRC})

target_include_directories(lib_ftl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_ftl PUBLIC
    bsp_nand
    lib_malloc)
