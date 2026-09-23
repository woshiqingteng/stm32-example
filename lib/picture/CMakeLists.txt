# ALIENTEK PICTURE middleware: BMP / JPEG / GIF viewer over the RGB panel.
# JPEG decoding uses the TJpgDec module, JPEG encoding uses LibJPEG, and the
# FatFs application glue is linked here. Usable as a LIB component: apps list
# ${LIB_SRC_PICTURE} and the matching lib_picture target plus the LCD BSP
# sources and the FatFs core/port are pulled in automatically.
set(LIB_SRC_PICTURE     ${CMAKE_CURRENT_SOURCE_DIR}/piclib.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/bmp.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/gif.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/jpeg_dec.c CACHE INTERNAL "")
set(LIB_SRC_PICTURE_HAL "" CACHE INTERNAL "")
set(LIB_SRC_PICTURE_BSP ${BSP_SRC_LCD} CACHE INTERNAL "")

add_library(lib_picture STATIC ${LIB_SRC_PICTURE})
set_target_properties(lib_picture PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_picture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_picture PUBLIC
    bsp lib_text libjpeg tjpgd lib_malloc fatfs_core fatfs_port)

set(LIB_COMPONENTS ${LIB_COMPONENTS} PICTURE CACHE INTERNAL "" FORCE)
