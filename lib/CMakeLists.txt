# Library layer: reusable application middleware. Each library is an
# independent target (lib_<name>) selected by apps via LIB components.
# Dependency rule: lib may depend on module + bsp + port.

set(LIB_COMPONENTS "" CACHE INTERNAL "" FORCE)

# Components that map straight onto port/module targets (no lib_<name> wrapper).
set(LIB_SRC_FATFS          FATFS         CACHE INTERNAL "")
set(LIB_SRC_FATFS_TARGETS  fatfs_core fatfs_port CACHE INTERNAL "")
set(LIB_SRC_FATFS_HAL ""    CACHE INTERNAL "")
set(LIB_SRC_FATFS_BSP ${BSP_SRC_SDIO} ${BSP_SRC_NORFLASH} CACHE INTERNAL "")

set(LIB_SRC_USB_DEVICE          USB_DEVICE     CACHE INTERNAL "")
set(LIB_SRC_USB_DEVICE_TARGETS  usb_device usb_port CACHE INTERNAL "")
set(LIB_SRC_USB_DEVICE_HAL ""   CACHE INTERNAL "")
set(LIB_SRC_USB_DEVICE_BSP ""   CACHE INTERNAL "")

set(LIB_SRC_USB_HOST          USB_HOST      CACHE INTERNAL "")
set(LIB_SRC_USB_HOST_TARGETS  usb_host usb_port CACHE INTERNAL "")
set(LIB_SRC_USB_HOST_HAL ""   CACHE INTERNAL "")
set(LIB_SRC_USB_HOST_BSP ""   CACHE INTERNAL "")

set(LIB_SRC_DSP          DSP       CACHE INTERNAL "")
set(LIB_SRC_DSP_TARGETS  cmsis_dsp m CACHE INTERNAL "")
set(LIB_SRC_DSP_HAL ""   CACHE INTERNAL "")
set(LIB_SRC_DSP_BSP ""   CACHE INTERNAL "")

set(LIB_COMPONENTS ${LIB_COMPONENTS} FATFS USB_DEVICE USB_HOST DSP CACHE INTERNAL "" FORCE)

add_subdirectory(usmart)
add_subdirectory(malloc)
add_subdirectory(text)
add_subdirectory(picture)
add_subdirectory(audio)
add_subdirectory(mjpeg)

# lib_resolve(<libs-out> <hal-out> <bsp-out> <lib sources...>): map the given
# LIB source lists to their library/port/module targets plus the HAL/BSP sources
# the matched components require.
function(lib_resolve libs_var hal_var bsp_var)
    set(_libs "")
    set(_hal "")
    set(_bsp "")
    foreach(_comp ${LIB_COMPONENTS})
        set(_match TRUE)
        foreach(_f ${LIB_SRC_${_comp}})
            if(NOT (_f IN_LIST ARGN))
                set(_match FALSE)
            endif()
        endforeach()
        if(_match)
            if(DEFINED LIB_SRC_${_comp}_TARGETS)
                list(APPEND _libs ${LIB_SRC_${_comp}_TARGETS})
            else()
                string(TOLOWER ${_comp} _lc)
                list(APPEND _libs lib_${_lc})
            endif()
            list(APPEND _hal ${LIB_SRC_${_comp}_HAL})
            list(APPEND _bsp ${LIB_SRC_${_comp}_BSP})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _libs)
    list(REMOVE_DUPLICATES _hal)
    list(REMOVE_DUPLICATES _bsp)
    bsp_resolve_hal(_from_bsp ${_bsp})
    list(APPEND _hal ${_from_bsp})
    list(REMOVE_DUPLICATES _hal)
    set(${libs_var} ${_libs} PARENT_SCOPE)
    set(${hal_var} ${_hal} PARENT_SCOPE)
    set(${bsp_var} ${_bsp} PARENT_SCOPE)
endfunction()
