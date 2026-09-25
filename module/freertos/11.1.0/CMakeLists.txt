# Vendor module: FreeRTOS-Kernel (11.1.0). Kernel sources plus the Cortex-M4F
# GCC port and heap_4. The common configuration (FreeRTOSConfig_common.h) lives
# in config/ and is exposed PUBLIC so any consumer of FreeRTOS (kernel, bsp)
# sees it; each app supplies its own FreeRTOSConfig.h that includes it.
file(GLOB FREERTOS_CORE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(freertos STATIC
    ${FREERTOS_CORE}
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/MemMang/heap_4.c)

target_include_directories(freertos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/config)

target_link_libraries(freertos PUBLIC target_${MCU_FAMILY})
