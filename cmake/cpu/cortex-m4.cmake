# Cortex-M4F flags. Applied globally via the CMAKE_* cache (this file is
# included from cmake/CMakeLists.txt, before any target is created).

set(_cpu_flags "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS
    "${_cpu_flags} -std=gnu11 -ffunction-sections -fdata-sections -Wall -Wextra"
    CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS
    "${_cpu_flags} -Wl,--gc-sections -Wl,--no-warn-rwx-segments \
-specs=nano.specs -specs=nosys.specs -Wl,--print-memory-usage"
    CACHE STRING "" FORCE)
