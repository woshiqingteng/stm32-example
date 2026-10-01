# The FreeRTOS configuration (FreeRTOSConfig_common.h) lives here, in the port,
# mirroring the lwIP port (lwipopts.h). FreeRTOS sources resolve
# "FreeRTOSConfig.h" from the app dir and "FreeRTOSConfig_common.h" from here.
target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Application hooks (vApplicationStackOverflowHook). Self-contained on purpose:
# it is linked after `freertos` so the kernel's reference pulls the object.
add_library(freertos_port STATIC EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/vApplicationHooks.c)

# Match the MCU FPU ABI (hard-float) so the hook object links with the app.
target_link_libraries(freertos_port PRIVATE ${MCU_ARCH_INTERFACE})

# Force the hook symbol so its object is pulled even when another library
# (lwip_port, ...) reorders the link line.
target_link_options(freertos_port INTERFACE -Wl,-u,vApplicationStackOverflowHook)
