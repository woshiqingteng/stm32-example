# Cortex-M4F global compile/link flags (applied through the toolchain).

set(CMAKE_C_FLAGS_INIT
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
-std=gnu11 -ffunction-sections -fdata-sections -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG_INIT "-Og -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE_INIT "-O2 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
-Wl,--gc-sections -Wl,--no-warn-rwx-segments -specs=nano.specs -specs=nosys.specs \
-Wl,--print-memory-usage")
