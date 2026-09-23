# ALIENTEK PICTURE middleware: BMP / JPEG / GIF viewer over the RGB panel. JPEG
# decoding and encoding use the LibJPEG module. Usable as a LIB component: apps
# list ${LIB_SRC_PICTURE} and the matching lib_picture target and the LCD BSP
# sources are pulled in automatically.
set(LIB_SRC_PICTURE     ${CMAKE_CURRENT_SOURCE_DIR}/piclib.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/bmp.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/gif.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/jpeg_dec.c CACHE INTERNAL "")
set(LIB_SRC_PICTURE_HAL "" CACHE INTERNAL "")
set(LIB_SRC_PICTURE_BSP ${BSP_SRC_LCD} CACHE INTERNAL "")

add_library(lib_picture STATIC ${LIB_SRC_PICTURE})
set_target_properties(lib_picture PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_picture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# piclib.h pulls in the FatFs application glue (exfuns.h -> ff.h); the archive
# itself is linked by the app so only the header search paths are needed here.
target_include_directories(lib_picture PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:fatfs_port,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(lib_picture PUBLIC bsp lib_text libjpeg lib_malloc)

set(LIB_COMPONENTS ${LIB_COMPONENTS} PICTURE CACHE INTERNAL "" FORCE)
