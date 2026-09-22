# ALIENTEK MJPEG middleware: AVI (Motion JPEG + PCM) video player with
# synchronised audio. Usable as a LIB component: apps list ${LIB_SRC_MJPEG} and
# the matching lib_mjpeg target plus the LCD BSP sources are pulled in
# automatically. JPEG frames decode through the LibJPEG module and audio
# playback through the AUDIO library.
set(LIB_SRC_MJPEG     ${CMAKE_CURRENT_SOURCE_DIR}/mjpeg.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/avi.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/videoplayer.c CACHE INTERNAL "")
set(LIB_SRC_MJPEG_HAL ${HAL_SRC_TIM} CACHE INTERNAL "")
set(LIB_SRC_MJPEG_BSP ${BSP_SRC_LCD} CACHE INTERNAL "")

add_library(lib_mjpeg STATIC ${LIB_SRC_MJPEG})
set_target_properties(lib_mjpeg PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_mjpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# videoplayer.c talks to the FatFs application glue (exfuns.h -> ff.h); the
# archive itself is linked by the app so only the header search paths are needed.
target_include_directories(lib_mjpeg PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:fatfs_port,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(lib_mjpeg PUBLIC bsp lib_text libjpeg lib_audio lib_malloc ${TARGET_LIB})

set(LIB_COMPONENTS ${LIB_COMPONENTS} MJPEG CACHE INTERNAL "" FORCE)
