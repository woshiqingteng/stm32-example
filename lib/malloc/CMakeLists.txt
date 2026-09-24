# ALIENTEK dynamic memory manager (internal SRAM, CCM and external SDRAM).
set(MALLOC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/malloc.c)

add_library(lib_malloc STATIC ${MALLOC_SRC})

target_include_directories(lib_malloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_malloc PUBLIC bsp)
