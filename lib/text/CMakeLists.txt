# ALIENTEK TEXT middleware: GBK text rendering over the RGB panel plus the
# NOR-flash GBK font store. Usable as a LIB component: apps list
# ${LIB_SRC_TEXT} and the matching lib_text target, the LCD BSP sources and the
# FatFs core/port are pulled in automatically.
set(LIB_SRC_TEXT     ${CMAKE_CURRENT_SOURCE_DIR}/text.c
                     ${CMAKE_CURRENT_SOURCE_DIR}/fonts.c CACHE INTERNAL "")
set(LIB_SRC_TEXT_HAL "" CACHE INTERNAL "")
set(LIB_SRC_TEXT_BSP ${BSP_SRC_LCD} ${BSP_SRC_NORFLASH} ${BSP_SRC_SDIO} CACHE INTERNAL "")

add_library(lib_text STATIC ${LIB_SRC_TEXT})
set_target_properties(lib_text PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_text PUBLIC bsp lib_malloc fatfs_core fatfs_port)

set(LIB_COMPONENTS ${LIB_COMPONENTS} TEXT CACHE INTERNAL "" FORCE)
