# Module layer: third-party / vendor components. Selection comes from the board
# manifest (see cmake/board/<BOARD>.cmake); nothing here is board hard-coded.
#
# Layout: module/<name>/<version>/  (CMSIS core, HAL, FreeRTOS, FatFs, ...).

set(MODULE_CMSIS_CORE_ENABLE ON  CACHE BOOL "Enable cmsis_core")
set(MODULE_CMSIS_DSP_ENABLE  ON  CACHE BOOL "Enable the CMSIS-DSP module")
set(MODULE_HAL_ENABLE        ON  CACHE BOOL "Enable the HAL module")
set(MODULE_FATFS_ENABLE      ON  CACHE BOOL "Enable the FatFs core module")
set(MODULE_LIBJPEG_ENABLE    ON  CACHE BOOL "Enable the LibJPEG module")
set(MODULE_HELIX_MP3_ENABLE  ON  CACHE BOOL "Enable the Helix MP3 decoder module")
set(MODULE_USB_DEVICE_ENABLE ON  CACHE BOOL "Enable the ST USB device stack")
set(MODULE_USB_HOST_ENABLE   ON  CACHE BOOL "Enable the ST USB host stack")

set(MODULE_CMSIS_CORE_VERSION 6.1.0  CACHE STRING "cmsis_core version")
set(MODULE_CMSIS_DSP_VERSION  1.17.1 CACHE STRING "cmsis_dsp version")
set(MODULE_FATFS_VERSION      r0.16  CACHE STRING "fatfs_core version")
set(MODULE_LIBJPEG_VERSION    8d     CACHE STRING "libjpeg version")
set(MODULE_HELIX_MP3_VERSION  1.0    CACHE STRING "helix_mp3 version")
set(MODULE_USB_DEVICE_VERSION 2.11.4 CACHE STRING "usb_device version")
set(MODULE_USB_HOST_VERSION   3.5.3  CACHE STRING "usb_host version")
set(MODULE_FREERTOS_VERSION   11.1.0 CACHE STRING "freertos version")

# module_req(<name>): require module/<name> to exist, else fail with a clear message.
function(module_req name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name})
        message(FATAL_ERROR "Missing module: module/${name}")
    endif()
endfunction()

if(MODULE_CMSIS_CORE_ENABLE)
    set(_m cmsis_core/${MODULE_CMSIS_CORE_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_CMSIS_DSP_ENABLE)
    set(_m cmsis_dsp/${MODULE_CMSIS_DSP_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_HAL_ENABLE)
    set(_m ${HAL_MODULE_DIR}/${HAL_MODULE_VERSION})
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_m})
        message(FATAL_ERROR
            "No HAL module for family '${MCU_FAMILY}': module/${_m}")
    endif()
    add_subdirectory(${_m})
endif()

if(MODULE_FATFS_ENABLE)
    set(_m fatfs_core/${MODULE_FATFS_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_LIBJPEG_ENABLE)
    set(_m libjpeg/${MODULE_LIBJPEG_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_HELIX_MP3_ENABLE)
    set(_m helix_mp3/${MODULE_HELIX_MP3_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_USB_DEVICE_ENABLE)
    set(_m usb_device/${MODULE_USB_DEVICE_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

if(MODULE_USB_HOST_ENABLE)
    set(_m usb_host/${MODULE_USB_HOST_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()

set(MODULE_FREERTOS_ENABLE OFF)
if(OS STREQUAL "freertos")
    set(MODULE_FREERTOS_ENABLE ON)
endif()

if(MODULE_FREERTOS_ENABLE)
    set(_m freertos/${MODULE_FREERTOS_VERSION})
    module_req(${_m})
    add_subdirectory(${_m})
endif()
