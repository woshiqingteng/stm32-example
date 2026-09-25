# Architecture layer: CPU core support, selected by the MCU architecture
# (Cortex-M uses Arm CMSIS-Core).
if(MCU_ARCH MATCHES "^cortex-m")
    add_subdirectory(cmsis_core)
else()
    message(FATAL_ERROR "Unsupported MCU arch: ${MCU_ARCH}")
endif()
