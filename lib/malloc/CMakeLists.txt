# ALIENTEK dynamic memory manager (internal SRAM, CCM and external SDRAM).
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_malloc STATIC ${_src})
set_target_properties(lib_malloc PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_malloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_malloc PUBLIC bsp)

set(LIB_SRC_MALLOC MALLOC CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} MALLOC CACHE INTERNAL "" FORCE)
