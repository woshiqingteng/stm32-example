file(GLOB MBEDTLS_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/library/*.c)

add_library(mbedtls STATIC EXCLUDE_FROM_ALL ${MBEDTLS_SRC})

target_include_directories(mbedtls PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config)

# The project supplies a bare-metal config (see config/mbedtls_config.h).
target_compile_definitions(mbedtls PUBLIC
    MBEDTLS_CONFIG_FILE="mbedtls_config.h")

# drv_core supplies the FPU ABI flags.
target_link_libraries(mbedtls PUBLIC drv_core)

# Third-party library: keep the project's -Wall/-Wextra out of its build.
target_compile_options(mbedtls PRIVATE -w)
