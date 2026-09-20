# Select the board (which fixes the CPU) and the toolchain, then global settings.

include("${CMAKE_CURRENT_LIST_DIR}/board/${BOARD}.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/toolchain/arm_${TOOLCHAIN}.cmake")

if(OS STREQUAL "freertos")
    set(BSP_SUPPORT_OS 1 CACHE INTERNAL "" FORCE)
else()
    set(BSP_SUPPORT_OS 0 CACHE INTERNAL "" FORCE)
endif()
