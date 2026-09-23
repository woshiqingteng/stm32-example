# ALIENTEK MJPEG middleware: AVI (Motion JPEG + PCM) video player with
# synchronised, self-contained ES8388 + SAI audio. JPEG frames decode through
# LibJPEG.
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_mjpeg STATIC ${_src})
set_target_properties(lib_mjpeg PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_mjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_mjpeg PUBLIC
    bsp lib_text libjpeg lib_malloc fatfs_core fatfs_port)

set(LIB_SRC_MJPEG MJPEG CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} MJPEG CACHE INTERNAL "" FORCE)
