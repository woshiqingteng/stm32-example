file(GLOB LWIP_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ipv4/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ipv6/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/netif/*.c)

add_library(lwip STATIC EXCLUDE_FROM_ALL ${LWIP_SRC})

target_include_directories(lwip PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# drv_core supplies the FPU ABI flags (and HAL headers).
target_link_libraries(lwip PUBLIC drv_core)

# Third-party stack: keep the project's -Wall/-Wextra out of its build.
target_compile_options(lwip PRIVATE -w)
