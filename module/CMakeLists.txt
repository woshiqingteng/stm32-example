# Module layer: third-party / vendor components. Each module directory owns its
# version selection (module/<name>/CMakeLists.txt sets <name>_VERSION and adds
# the versioned subdirectory).
#
# Dependency rule: modules depend only on the platform base and other modules;
# never on bsp/port/lib/app.
#
# Enable flags are CACHE variables so sibling layers (port, bsp) can read them.
# Only the HAL and the RTOS are genuine selections (driven by BSP_DRV / OS); the
# other vendor modules are always added -- whether they are built is decided by
# EXCLUDE_FROM_ALL and the app's dependency closure.

set(MODULE_HAL_ENABLE        OFF CACHE BOOL "Enable the HAL module")
set(MODULE_FREERTOS_ENABLE   OFF CACHE BOOL "Enable FreeRTOS")

if(BSP_DRV STREQUAL "stm32_hal")
    set(MODULE_HAL_ENABLE ON CACHE BOOL "" FORCE)
endif()
if(OS STREQUAL "freertos")
    set(MODULE_FREERTOS_ENABLE ON CACHE BOOL "" FORCE)
endif()

if(MODULE_HAL_ENABLE)
    add_subdirectory(stm32_hal/${MCU_FAMILY})
endif()

if(MODULE_FREERTOS_ENABLE)
    add_subdirectory(freertos)
endif()

add_subdirectory(fatfs)
add_subdirectory(libjpeg)
add_subdirectory(tjpgd)
add_subdirectory(cmsis_dsp)
add_subdirectory(usb_device)
add_subdirectory(usb_host)
