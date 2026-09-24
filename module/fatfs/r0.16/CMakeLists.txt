# Vendor module: FatFs (R0.16) core filesystem. Board independent; the physical
# drive mapping lives in the port layer (port/fatfs_port).
add_library(fatfs_core_inc INTERFACE)
target_include_directories(fatfs_core_inc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(fatfs_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ffunicode.c)
target_link_libraries(fatfs_core PUBLIC fatfs_core_inc target_${MCU_FAMILY})
