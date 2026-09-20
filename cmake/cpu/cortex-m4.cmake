# Cortex-M4F flags, set globally before any target is created.

set(CMAKE_C_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
-std=gnu11 -ffunction-sections -fdata-sections -Wall -Wextra"
    CACHE STRING "" FORCE)

set(CMAKE_C_FLAGS_DEBUG   "-Og -g3 -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG"    CACHE STRING "" FORCE)

set(CMAKE_EXE_LINKER_FLAGS
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
-Wl,--gc-sections -Wl,--no-warn-rwx-segments -specs=nano.specs -specs=nosys.specs \
-Wl,--print-memory-usage"
    CACHE STRING "" FORCE)
