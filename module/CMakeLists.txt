# Module layer: third-party / vendor components. Each module directory owns its
# version selection (module/<name>/CMakeLists.txt sets <name>_VERSION and adds
# the versioned subdirectory).
#
# Dependency rule: modules depend only on the platform base and other modules;
# never on bsp/port/lib/app.
#
# Enable flags are CACHE variables so sibling layers (port, bsp) can read them.

set(MODULE_HAL_ENABLE        OFF CACHE BOOL "Enable the HAL module")
set(MODULE_FREERTOS_ENABLE   OFF CACHE BOOL "Enable FreeRTOS")
set(MODULE_FATFS_ENABLE      ON  CACHE BOOL "Enable the FatFs core module")
set(MODULE_LIBJPEG_ENABLE    ON  CACHE BOOL "Enable the LibJPEG module")
set(MODULE_TJPGD_ENABLE      ON  CACHE BOOL "Enable the TJpgDec module")
set(MODULE_CMSIS_DSP_ENABLE  ON  CACHE BOOL "Enable the CMSIS-DSP module")
set(MODULE_USB_DEVICE_ENABLE ON  CACHE BOOL "Enable the ST USB device stack")
set(MODULE_USB_HOST_ENABLE   ON  CACHE BOOL "Enable the ST USB host stack")

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

if(MODULE_FATFS_ENABLE)
    add_subdirectory(fatfs)
endif()

if(MODULE_LIBJPEG_ENABLE)
    add_subdirectory(libjpeg)
endif()

if(MODULE_TJPGD_ENABLE)
    add_subdirectory(tjpgd)
endif()

if(MODULE_CMSIS_DSP_ENABLE)
    add_subdirectory(cmsis_dsp)
endif()

if(MODULE_USB_DEVICE_ENABLE)
    add_subdirectory(usb_device)
endif()

if(MODULE_USB_HOST_ENABLE)
    add_subdirectory(usb_host)
endif()
