# Module layer: third-party / vendor components. Selection comes from the board
# manifest (see cmake/board/<BOARD>.cmake); nothing here is board hard-coded.

set(MODULE_CMSIS_CORE_ENABLE ON  CACHE BOOL "Enable cmsis_core")
set(MODULE_HAL_ENABLE        ON  CACHE BOOL "Enable the HAL module")
set(MODULE_FATFS_ENABLE      ON  CACHE BOOL "Enable the FatFs core module")
set(MODULE_LIBJPEG_ENABLE    ON  CACHE BOOL "Enable the LibJPEG module")
set(MODULE_HELIX_MP3_ENABLE  ON  CACHE BOOL "Enable the Helix MP3 decoder module")

if(MODULE_CMSIS_CORE_ENABLE)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmsis_core/${CMSIS_CORE_VERSION})
        message(FATAL_ERROR "No cmsis_core module: module/cmsis_core/${CMSIS_CORE_VERSION}")
    endif()
    add_subdirectory(cmsis_core/${CMSIS_CORE_VERSION})
endif()

if(MODULE_HAL_ENABLE)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${HAL_MODULE_DIR})
        message(FATAL_ERROR
            "No HAL module for series '${STM32_SERIES}': module/${HAL_MODULE_DIR}")
    endif()
    add_subdirectory(${HAL_MODULE_DIR})
endif()

if(MODULE_FATFS_ENABLE)
    add_subdirectory(fatfs_core)
endif()

if(MODULE_LIBJPEG_ENABLE)
    add_subdirectory(libjpeg)
endif()

if(MODULE_HELIX_MP3_ENABLE)
    add_subdirectory(helix_mp3)
endif()

if(OS STREQUAL "freertos")
    set(MODULE_FREERTOS_ENABLE ON  CACHE BOOL "Enable freertos" FORCE)
else()
    set(MODULE_FREERTOS_ENABLE OFF CACHE BOOL "Enable freertos" FORCE)
endif()

if(MODULE_FREERTOS_ENABLE)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/freertos/${MODULE_FREERTOS_VERSION})
        message(FATAL_ERROR "No freertos module: module/freertos/${MODULE_FREERTOS_VERSION}")
    endif()
    add_subdirectory(freertos/${MODULE_FREERTOS_VERSION})
endif()
