# Vendor module: FatFs (R0.16) core filesystem. Board independent; the physical
# drive mapping lives in the port layer (port/fatfs_port).
set(FATFS_CORE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ffunicode.c)

add_library(fatfs_core STATIC ${FATFS_CORE_SRC})
set_target_properties(fatfs_core PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(fatfs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fatfs_core PUBLIC ${TARGET_LIB})
