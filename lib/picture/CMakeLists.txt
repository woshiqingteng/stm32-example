# ALIENTEK PICTURE middleware: BMP / JPEG / GIF viewer over the RGB panel.
# JPEG decoding uses TJpgDec, JPEG encoding uses LibJPEG.
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_picture STATIC ${_src})
set_target_properties(lib_picture PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_picture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_picture PUBLIC
    bsp lib_text libjpeg tjpgd lib_malloc fatfs_core fatfs_port)

set(LIB_SRC_PICTURE PICTURE CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} PICTURE CACHE INTERNAL "" FORCE)
