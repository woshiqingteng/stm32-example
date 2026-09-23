# ALIENTEK WAV player middleware: ES8388 + SAI playback device plus WAV
# decoding. The playlist/navigation lives in the 46_music app. Usable as a LIB
# component: apps list ${LIB_SRC_AUDIO} and the matching lib_audio target plus
# the ES8388/SAI/IIC BSP sources and the FatFs core/port are pulled in.
set(LIB_SRC_AUDIO     ${CMAKE_CURRENT_SOURCE_DIR}/wavplay.c CACHE INTERNAL "")
set(LIB_SRC_AUDIO_HAL "" CACHE INTERNAL "")
set(LIB_SRC_AUDIO_BSP ${BSP_SRC_ES8388} ${BSP_SRC_SAI} ${BSP_SRC_IIC} CACHE INTERNAL "")

add_library(lib_audio STATIC ${LIB_SRC_AUDIO})
set_target_properties(lib_audio PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_audio PUBLIC
    bsp lib_text lib_malloc fatfs_core fatfs_port)

set(LIB_COMPONENTS ${LIB_COMPONENTS} AUDIO CACHE INTERNAL "" FORCE)
