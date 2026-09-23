# ALIENTEK WAV player middleware: ES8388 + SAI playback device plus WAV
# decoding and printf status. The playlist/navigation lives in the 46_music app.
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_audio STATIC ${_src})
set_target_properties(lib_audio PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_audio PUBLIC
    bsp lib_malloc fatfs_core fatfs_port)

set(LIB_SRC_AUDIO AUDIO CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} AUDIO CACHE INTERNAL "" FORCE)
