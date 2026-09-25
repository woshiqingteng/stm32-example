# ALIENTEK WAV player middleware: ES8388 + SAI playback device plus WAV
# decoding and printf status. The playlist/navigation lives in the 46_music app.
set(AUDIO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/wavplay.c)

add_library(lib_audio STATIC EXCLUDE_FROM_ALL ${AUDIO_SRC})

target_include_directories(lib_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_audio PUBLIC bsp lib_malloc fatfs_core fatfs_port)
