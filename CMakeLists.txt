cmake_minimum_required(VERSION 3.24)

if(NOT TOOLCHAIN)
    message(FATAL_ERROR "TOOLCHAIN not set")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain/${TOOLCHAIN}.cmake)
message(STATUS "Toolchain : ${TOOLCHAIN}")

if(NOT BOARD)
    message(FATAL_ERROR "BOARD not set")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/cmake/board/${BOARD}.cmake)
message(STATUS "Board : ${BOARD}")

if(NOT APP_TARGET MATCHES "^app_([^_]+)_(.+)$")
    message(FATAL_ERROR "Invalid APP_TARGET: ${APP_TARGET} (expect app_<os>_<name>)")
endif()
set(OS ${CMAKE_MATCH_1})
set(APP_NAME ${CMAKE_MATCH_2})
message(STATUS "OS : ${OS} App : ${APP_NAME}")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/func/flow.cmake)

project(${APP_TARGET} LANGUAGES C CXX ASM)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(platform)
add_subdirectory(module)
add_subdirectory(bsp)
add_subdirectory(port)
add_subdirectory(lib)
add_subdirectory(app)
