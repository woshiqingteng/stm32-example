# Device selection. The board fixes the CPU; this layer fixes the device.

set(TARGET_DIR "${CMAKE_CURRENT_LIST_DIR}/stm32f4" CACHE INTERNAL "" FORCE)
set(DEVICE_DEFINE "STM32F429xx" CACHE INTERNAL "" FORCE)
set(TARGET_LD_SCRIPT "${TARGET_DIR}/stm32f429igtx_flash.ld" CACHE INTERNAL "" FORCE)

set(TARGET_STM32F4
    ${TARGET_DIR}/startup_stm32f429igtx.c
    ${TARGET_DIR}/vector_stm32f429igtx.c
    ${TARGET_DIR}/system_stm32f4xx.c
    ${TARGET_DIR}/stm32f4xx_it.c
    CACHE INTERNAL "" FORCE)
