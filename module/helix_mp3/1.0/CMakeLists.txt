# Vendor module: Helix fixed-point MP3 decoder (RealNetworks). Board
# independent; used by the AUDIO library for MP3 playback. The layout keeps the
# upstream src/ split: the public header (pub/) plus the portable decoder core
# (real/). The architecture specific real/arm assembly is intentionally not
# compiled; the portable C fallback in real/assembly.h is used instead.
set(HELIX_MP3_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/mp3dec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mp3tabs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/bitstream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/buffers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/dct32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/dequant.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/dqchan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/huffman.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/hufftabs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/imdct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/polyphase.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/scalfact.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/stproc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/subband.c
    ${CMAKE_CURRENT_SOURCE_DIR}/real/trigtabs.c)

add_library(helix_mp3 STATIC ${HELIX_MP3_SRC})
set_target_properties(helix_mp3 PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(helix_mp3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pub)
target_link_libraries(helix_mp3 PUBLIC ${TARGET_LIB})
