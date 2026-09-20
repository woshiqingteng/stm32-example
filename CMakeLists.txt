cmake_minimum_required(VERSION 3.24)

project(stm32_example LANGUAGES C)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/CMakeLists.txt)

include(${CMAKE_CURRENT_LIST_DIR}/module/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/target/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/bsp/CMakeLists.txt)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/app)
