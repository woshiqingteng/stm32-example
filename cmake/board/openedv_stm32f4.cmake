# Board manifest: the single source of truth for the chip, the HAL/CMSIS
# modules and the board support package selected by BOARD.

set(CPU cortex-m4)
include(${CMAKE_CURRENT_LIST_DIR}/../cpu/${CPU}.cmake)

# --- chip ---
set(STM32_SERIES       stm32f4          CACHE INTERNAL "" FORCE)
set(STM32_PART         STM32F429IGTx    CACHE INTERNAL "" FORCE)
set(STM32_DEVICE_MACRO STM32F429xx      CACHE INTERNAL "" FORCE)
set(STM32_HSE_VALUE    25000000         CACHE INTERNAL "" FORCE)
set(STM32_HSI_VALUE    16000000         CACHE INTERNAL "" FORCE)
set(MCU_FLASH_BASE     0x08000000       CACHE INTERNAL "" FORCE)

# --- HAL / CMSIS modules ---
set(HAL_MODULE_DIR     stm32_hal/f4xx        CACHE INTERNAL "" FORCE)
set(HAL_MODULE_VERSION 1.8.5                CACHE INTERNAL "" FORCE)
set(MODULE_FREERTOS_VERSION 11.1.0          CACHE INTERNAL "" FORCE)

# --- chip support package (target/<vendor>/<family>) ---
set(STM32_TARGET_DIR  stm32/f4xx            CACHE INTERNAL "" FORCE)
set(TARGET_LIB        target_${STM32_SERIES} CACHE INTERNAL "" FORCE)
set(STM32_STARTUP_SRC startup_stm32f429ig.c  CACHE INTERNAL "" FORCE)
set(STM32_VECTOR_SRC  vector_stm32f429ig.c   CACHE INTERNAL "" FORCE)
set(STM32_SYSTEM_SRC  system_stm32f4xx.c     CACHE INTERNAL "" FORCE)
set(STM32_IT_SRC      stm32f4xx_it.c         CACHE INTERNAL "" FORCE)
set(STM32_LD_FLASH    stm32f429ig_flash.ld   CACHE INTERNAL "" FORCE)
set(STM32_LD_RAM      stm32f429ig_ram.ld     CACHE INTERNAL "" FORCE)

# --- board support package ---
set(BSP_BOARD_DIR openedv_stm32f4 CACHE INTERNAL "" FORCE)

# The board manifest is the single source of truth: it must define the full
# chip/HAL/BSP/target selection so the lower layers stay board-agnostic.
foreach(_v
        BOARD CPU STM32_SERIES STM32_PART STM32_DEVICE_MACRO
        HAL_MODULE_DIR STM32_TARGET_DIR BSP_BOARD_DIR TARGET_LIB)
    if(NOT ${_v})
        message(FATAL_ERROR "BOARD '${BOARD}': manifest variable '${_v}' not set")
    endif()
endforeach()

message(STATUS "Board     : ${BOARD}")
message(STATUS "Chip      : ${STM32_PART} (${STM32_DEVICE_MACRO})")
message(STATUS "HAL       : module/${HAL_MODULE_DIR}/${HAL_MODULE_VERSION}")
message(STATUS "BSP       : bsp/${BSP_BOARD_DIR}")
message(STATUS "Target    : target/${STM32_TARGET_DIR}")
