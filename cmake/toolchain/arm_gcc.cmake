# arm-none-eabi-gcc toolchain. Global compile/link flags live in cpu/ and are
# pulled in here (before project() enables the language).

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# BOARD/CPU come from the preset cache; provide defaults for nested try_compile.
if(NOT BOARD)
    set(BOARD "openedv_stm32f4")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/../board/${BOARD}.cmake")

if(NOT CPU)
    set(CPU "cortex-m4")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/../cpu/${CPU}.cmake")
