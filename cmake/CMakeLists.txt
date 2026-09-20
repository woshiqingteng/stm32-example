# Board selection (the board fixes the CPU), CPU flags, then the toolchain.
# The preset still points CMAKE_TOOLCHAIN_FILE at this directory's toolchain file.

include("${CMAKE_CURRENT_LIST_DIR}/board/${BOARD}.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cpu/${CPU}.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/toolchain/arm_${TOOLCHAIN}.cmake")

if(OS STREQUAL "freertos")
    set(BSP_SUPPORT_OS 1 CACHE INTERNAL "" FORCE)
else()
    set(BSP_SUPPORT_OS 0 CACHE INTERNAL "" FORCE)
endif()
