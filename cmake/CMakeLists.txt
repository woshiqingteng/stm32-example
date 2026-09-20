# Board / CPU / function selection.

set(TOOLCHAIN "gcc" CACHE STRING "Toolchain: gcc")
set(BOARD "openedv_stm32f4" CACHE STRING "Board")
set(OS "baremetal" CACHE STRING "OS: baremetal or freertos")

set(MCU_GEN_FULL_ARTIFACT OFF CACHE BOOL "Also generate .hex and .lst")

include(${CMAKE_CURRENT_LIST_DIR}/board/${BOARD}.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cpu/${CPU}.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/func/flow.cmake)

if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
endif()
if(NOT CMAKE_OBJDUMP)
    find_program(CMAKE_OBJDUMP arm-none-eabi-objdump REQUIRED)
endif()

if(OS STREQUAL "freertos")
    add_compile_definitions(BSP_SUPPORT_OS=1)
else()
    add_compile_definitions(BSP_SUPPORT_OS=0)
endif()
