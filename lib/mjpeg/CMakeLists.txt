# ALIENTEK MJPEG middleware: AVI (Motion JPEG + PCM) video player with
# synchronised audio. Usable as a LIB component: apps list ${LIB_SRC_MJPEG} and
# the matching lib_mjpeg target plus the LCD BSP sources and the FatFs core/port
# are pulled in automatically. JPEG frames decode through LibJPEG; the audio
# path is self-contained (ES8388 + SAI BSP).
set(LIB_SRC_MJPEG     ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/avi.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/videoplayer.c CACHE INTERNAL "")
set(LIB_SRC_MJPEG_BSP ${BSP_SRC_LCD} ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_IIC} ${BSP_SRC_SDIO} ${BSP_SRC_NORFLASH} CACHE INTERNAL "")

add_library(lib_mjpeg STATIC ${LIB_SRC_MJPEG})
set_target_properties(lib_mjpeg PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_mjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_mjpeg PUBLIC
    bsp lib_text libjpeg lib_malloc fatfs_core fatfs_port)

set(LIB_COMPONENTS ${LIB_COMPONENTS} MJPEG CACHE INTERNAL "" FORCE)
