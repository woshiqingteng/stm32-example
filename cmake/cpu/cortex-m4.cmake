# Cortex-M4 / Cortex-M4F compile and link flags.

option(CONFIG_FPU "Enable the Cortex-M4 FPU" ON)
option(CONFIG_HARD_FPU "Use the hard-float ABI" ON)

set(CPU_FLAGS -mcpu=cortex-m4 -mthumb)

if(CONFIG_FPU)
    list(APPEND CPU_FLAGS -mfpu=fpv4-sp-d16)
    if(CONFIG_HARD_FPU)
        list(APPEND CPU_FLAGS -mfloat-abi=hard)
    else()
        list(APPEND CPU_FLAGS -mfloat-abi=softfp)
    endif()
else()
    list(APPEND CPU_FLAGS -mfloat-abi=soft)
endif()

add_compile_options(${CPU_FLAGS} -std=gnu11 -ffunction-sections -fdata-sections -Wall -Wextra)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Og -g3 -DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2 -DNDEBUG)
endif()

add_link_options(${CPU_FLAGS} -Wl,--gc-sections -Wl,--no-warn-rwx-segments
                 -specs=nano.specs -specs=nosys.specs -Wl,--print-memory-usage)
