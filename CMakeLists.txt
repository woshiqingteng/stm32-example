cmake_minimum_required(VERSION 3.24)

if(NOT TOOLCHAIN)
    message(FATAL_ERROR "TOOLCHAIN not set")
endif()
include(cmake/toolchain/${TOOLCHAIN}.cmake)
message(STATUS "Toolchain : ${TOOLCHAIN}")

project(stm32-example LANGUAGES C ASM)

# Keep final artifacts (executables, static libraries) at the build root so
# intermediates and outputs share the same directory tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(cmake)
add_subdirectory(target)
add_subdirectory(module)
add_subdirectory(bsp)
add_subdirectory(app)
