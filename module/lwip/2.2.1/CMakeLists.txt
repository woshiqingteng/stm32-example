# lwIP 2.2.1 source (vendored). NOT wired into the build yet. This file is only
# a starting point for a future port.

file(GLOB_RECURSE LWIP_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/netif/*.c)

add_library(lwip STATIC EXCLUDE_FROM_ALL ${LWIP_SRC})

target_include_directories(lwip PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# lwipopts.h and arch/cc.h are provided by the future port.
target_link_libraries(lwip PUBLIC drv_core)
