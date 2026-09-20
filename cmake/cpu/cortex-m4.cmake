# Cortex-M4F flags (FPU switches + global C/link flags).

option(CONFIG_FPU      "Enable FPU instructions" ON)
option(CONFIG_HARD_FPU "Use hard-float ABI"     ON)

set(_mfpu -mfpu=fpv4-sp-d16)
if(NOT CONFIG_FPU)
    set(_mfpu "")
    set(_mfloat -mfloat-abi=soft)
elseif(CONFIG_HARD_FPU)
    set(_mfloat -mfloat-abi=hard)
else()
    set(_mfloat -mfloat-abi=softfp)
endif()

set(CMAKE_C_FLAGS
    "-mcpu=cortex-m4 -mthumb ${_mfpu} ${_mfloat} -std=gnu11 \
-ffunction-sections -fdata-sections -fno-builtin -fno-common -Wall -Wextra"
    CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_DEBUG   "-Og -g3 -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG"    CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS
    "-mcpu=cortex-m4 -mthumb ${_mfpu} ${_mfloat} \
-Wl,--gc-sections -Wl,--no-warn-rwx-segments -specs=nano.specs -specs=nosys.specs \
-Wl,--print-memory-usage"
    CACHE STRING "" FORCE)
