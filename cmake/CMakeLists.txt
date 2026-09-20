# Board / CPU selection happens in the toolchain file (global scope).
# This directory only exposes global cache values used by the layers.

if(NOT TOOLCHAIN STREQUAL "gcc")
    message(FATAL_ERROR "Unsupported TOOLCHAIN: ${TOOLCHAIN}")
endif()

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
find_program(CMAKE_OBJDUMP arm-none-eabi-objdump)

if(OS STREQUAL "freertos")
    set(BSP_SUPPORT_OS 1 CACHE INTERNAL "" FORCE)
else()
    set(BSP_SUPPORT_OS 0 CACHE INTERNAL "" FORCE)
endif()
