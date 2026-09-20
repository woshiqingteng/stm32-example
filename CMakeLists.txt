cmake_minimum_required(VERSION 3.24)

project(stm32_example LANGUAGES C)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/cmake)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/module)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/target)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/bsp)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/app)
