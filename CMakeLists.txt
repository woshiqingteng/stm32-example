cmake_minimum_required(VERSION 3.24)

if(NOT TOOLCHAIN)
    message(FATAL_ERROR "TOOLCHAIN not set")
endif()
include(cmake/toolchain/${TOOLCHAIN}.cmake)
message(STATUS "Toolchain : ${TOOLCHAIN}")

project(stm32-example LANGUAGES C ASM)

add_subdirectory(cmake)
add_subdirectory(target)
add_subdirectory(module)
add_subdirectory(bsp)
add_subdirectory(app)
