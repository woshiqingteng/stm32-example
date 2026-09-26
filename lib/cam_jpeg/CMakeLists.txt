set(SOURCES cam_jpeg.c)

add_library(lib_cam_jpeg STATIC EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(lib_cam_jpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_cam_jpeg PUBLIC bsp_dcmi bsp_sys)
