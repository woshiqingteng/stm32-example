# Device selection. Startup/system sources and the linker script come from the board.

set(TARGET_STM32F4
    ${STM32_STARTUP_SOURCE}
    ${STM32_VECTOR_SOURCE}
    ${STM32_IT_SOURCE}
    ${STM32_SYSTEM_SOURCE}
    CACHE INTERNAL "" FORCE)
