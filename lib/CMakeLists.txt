# wrapper
set(LIB_ALL_TARGET "")

function(lib_wrapper name)
    add_library(${name} INTERFACE)
    if(ARGN)
        target_link_libraries(${name} INTERFACE ${ARGN})
    endif()
    set(_all ${LIB_ALL_TARGET})
    list(APPEND _all ${name})
    set(LIB_ALL_TARGET ${_all} PARENT_SCOPE)
endfunction()

lib_wrapper(lib_fatfs
    fatfs_core fatfs_port)
lib_wrapper(lib_fatfs_stm32_usb_msc
    fatfs_core fatfs_stm32_usb_msc_port)
lib_wrapper(lib_stm32_usb_device_cdc
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_cdc_port)
lib_wrapper(lib_stm32_usb_device_audio
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_audio_port)
lib_wrapper(lib_stm32_usb_device_msc
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_msc_port)
lib_wrapper(lib_stm32_usb_host_hid
    stm32_usb_host stm32_usb_host_conf_port)
lib_wrapper(lib_stm32_usb_host_msc
    stm32_usb_host stm32_usb_host_conf_port stm32_usb_host_msc_port)
lib_wrapper(lib_dsp
    cmsis_dsp)

# lib all
function(lib_dir name)
    add_subdirectory(${name})
    set(_all ${LIB_ALL_TARGET})
    list(APPEND _all lib_${name})
    set(LIB_ALL_TARGET ${_all} PARENT_SCOPE)
endfunction()

foreach(_d usmart malloc text picture audio mjpeg ftl)
    lib_dir(${_d})
endforeach()

set(LIB_ALL_TARGET ${LIB_ALL_TARGET} CACHE INTERNAL "" FORCE)

# parse
function(lib_resolve out)
    if("all" IN_LIST ARGN)
        list(LENGTH ARGN _n)
        if(_n GREATER 1)
            message(FATAL_ERROR "LIB: 'all' must be used alone")
        endif()
        set(${out} ${LIB_ALL_TARGET} PARENT_SCOPE)
        return()
    endif()

    set(_r "")
    foreach(_n ${ARGN})
        string(TOLOWER ${_n} _lc)
        if(NOT TARGET lib_${_lc})
            message(FATAL_ERROR "Unknown LIB: '${_n}'")
        endif()
        list(APPEND _r lib_${_lc})
    endforeach()
    list(REMOVE_DUPLICATES _r)
    set(${out} ${_r} PARENT_SCOPE)
endfunction()
