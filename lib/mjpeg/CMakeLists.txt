# ALIENTEK MJPEG middleware: AVI (Motion JPEG + PCM) video player with
# synchronised, self-contained ES8388 + SAI audio. JPEG frames decode through
# LibJPEG.
set(MJPEG_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/avi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/videoplayer.c)

add_library(lib_mjpeg STATIC ${MJPEG_SRC})

target_include_directories(lib_mjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_mjpeg PUBLIC
    bsp lib_text libjpeg lib_malloc fatfs_core fatfs_port)
