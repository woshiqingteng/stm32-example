set(SOURCES imu.c)

add_library(lib_imu STATIC EXCLUDE_FROM_ALL ${SOURCES})
target_include_directories(lib_imu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_imu PUBLIC m ${MCU_ARCH_INTERFACE})
