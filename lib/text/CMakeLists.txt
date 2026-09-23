# ALIENTEK TEXT middleware: GBK text rendering over the RGB panel plus the
# NOR-flash GBK font store. Usable as a LIB component: apps list
# ${LIB_SRC_TEXT} and the matching lib_text target and the LCD BSP sources are
# pulled in automatically.
set(LIB_SRC_TEXT     ${CMAKE_CURRENT_SOURCE_DIR}/text.c
                     ${CMAKE_CURRENT_SOURCE_DIR}/fonts.c CACHE INTERNAL "")
set(LIB_SRC_TEXT_HAL "" CACHE INTERNAL "")
set(LIB_SRC_TEXT_BSP ${BSP_SRC_LCD} CACHE INTERNAL "")

add_library(lib_text STATIC ${LIB_SRC_TEXT})
set_target_properties(lib_text PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# fonts.c talks to the FatFs core directly to pull the font files off the SD.
target_include_directories(lib_text PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(lib_text PUBLIC bsp lib_malloc)

set(LIB_COMPONENTS ${LIB_COMPONENTS} TEXT CACHE INTERNAL "" FORCE)
