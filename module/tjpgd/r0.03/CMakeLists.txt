# Vendor module: ChaN TJpgDec R0.03 (tiny JPEG decompressor, decode only).
# Board independent; requires only the C standard headers.
set(TJPGD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tjpgd.c)

add_library(tjpgd STATIC ${TJPGD_SRC})
set_target_properties(tjpgd PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(tjpgd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tjpgd PUBLIC ${TARGET_LIB})
