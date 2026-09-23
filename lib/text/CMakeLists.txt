# ALIENTEK TEXT middleware: GBK text rendering over the RGB panel plus the
# NOR-flash GBK font store.
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_text STATIC ${_src})
set_target_properties(lib_text PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_text PUBLIC bsp lib_malloc fatfs_core fatfs_port)

set(LIB_SRC_TEXT TEXT CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} TEXT CACHE INTERNAL "" FORCE)
