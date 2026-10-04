set(MJPEG_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/avi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/videoplayer.c)

add_library(lib_mjpeg STATIC EXCLUDE_FROM_ALL ${MJPEG_SRC})

target_include_directories(lib_mjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_mjpeg PUBLIC
    bsp_codec bsp_sai bsp_tim
    lib_text libjpeg lib_malloc fatfs_core fatfs_port)
