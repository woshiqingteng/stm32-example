# CherryUSB v1.6.1 source (vendored, third_party/ excluded). NOT wired into the
# build yet. This file is only a starting point for a future port.

file(GLOB_RECURSE CHERRYUSB_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/class/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osal/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/port/*.c)

add_library(cherryusb STATIC EXCLUDE_FROM_ALL ${CHERRYUSB_SRC})

target_include_directories(cherryusb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/class
    ${CMAKE_CURRENT_SOURCE_DIR}/osal
    ${CMAKE_CURRENT_SOURCE_DIR}/port)

# cherryusb_config.h + the MCU port sources are chosen by the future port.
target_link_libraries(cherryusb PUBLIC drv_core)
