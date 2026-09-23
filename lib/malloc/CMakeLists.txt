# ALIENTEK dynamic memory manager. Usable as a LIB component: apps list
# ${LIB_SRC_MALLOC} and the matching lib_malloc target plus the SDRAM BSP
# sources are pulled in automatically.
set(LIB_SRC_MALLOC     ${CMAKE_CURRENT_SOURCE_DIR}/malloc.c CACHE INTERNAL "")
set(LIB_SRC_MALLOC_BSP ${BSP_SRC_SDRAM} CACHE INTERNAL "")

add_library(lib_malloc STATIC ${LIB_SRC_MALLOC})
set_target_properties(lib_malloc PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_malloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_malloc PUBLIC bsp)

set(LIB_COMPONENTS ${LIB_COMPONENTS} MALLOC CACHE INTERNAL "" FORCE)
