# ALIENTEK AUDIOCODEC middleware: ES8388 + SAI playback (WAV/MP3) and WAV
# recording. Usable as a LIB component: apps list ${LIB_SRC_AUDIO} and the
# matching lib_audio target plus the ES8388/SAI/IIC BSP sources are pulled in
# automatically. MP3 decoding uses the Helix module.
set(LIB_SRC_AUDIO     ${CMAKE_CURRENT_SOURCE_DIR}/audio.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/wavplay.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/mp3play.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/recorder.c CACHE INTERNAL "")
set(LIB_SRC_AUDIO_HAL "" CACHE INTERNAL "")
set(LIB_SRC_AUDIO_BSP ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_IIC} CACHE INTERNAL "")

add_library(lib_audio STATIC ${LIB_SRC_AUDIO})
set_target_properties(lib_audio PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# audio.c talks to the FatFs application glue (exfuns.h -> ff.h); the archive
# itself is linked by the app so only the header search paths are needed here.
target_include_directories(lib_audio PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:fatfs_port,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(lib_audio PUBLIC bsp lib_text lib_malloc helix_mp3)

set(LIB_COMPONENTS ${LIB_COMPONENTS} AUDIO CACHE INTERNAL "" FORCE)
