# ALIENTEK PICTURE middleware: BMP / JPEG / GIF viewer over the RGB panel.
# JPEG decoding uses TJpgDec, JPEG encoding uses LibJPEG.
set(PICTURE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/piclib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/jpeg_dec.c)

add_library(lib_picture STATIC EXCLUDE_FROM_ALL ${PICTURE_SRC})

target_include_directories(lib_picture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_picture PUBLIC
    bsp lib_text libjpeg tjpgd lib_malloc fatfs_core fatfs_port)
