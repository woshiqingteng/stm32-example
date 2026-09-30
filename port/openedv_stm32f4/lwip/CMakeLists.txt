# The lwip sources resolve "lwipopts.h" and "arch/cc.h" from this port dir.
# PRIVATE: do not leak the port's config headers into the module's interface.
target_include_directories(lwip PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lwip_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/sys_arch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ethernetif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip_comm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip_crypto.c)

target_include_directories(lwip_port PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/arch)

target_link_libraries(lwip_port PUBLIC
    lwip bsp_eth bsp_delay mbedtls)

# The port is FreeRTOS-only; freertos is absent on bare-metal builds, where this
# target is never linked.
if(TARGET freertos)
    target_link_libraries(lwip_port PUBLIC freertos)
endif()
