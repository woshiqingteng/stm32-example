# Port layer: FatFs physical drive glue (diskio) plus the small application
# glue (exfuns). Depends on the board BSP and the FatFs core headers.
set(FATFS_PORT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/exfuns.c)

add_library(fatfs_port STATIC ${FATFS_PORT_SRC})

target_include_directories(fatfs_port PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# FatFs core headers only. The core library is intentionally not a link
# dependency: diskio.c (here) and ff.c (core) reference each other, so the
# consumer must link fatfs_core before fatfs_port to keep static archive order.
target_include_directories(fatfs_port PRIVATE
    $<TARGET_PROPERTY:fatfs_core,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(fatfs_port PUBLIC bsp)
