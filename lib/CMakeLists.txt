add_library(lib_fatfs INTERFACE)
target_link_libraries(lib_fatfs INTERFACE fatfs_core fatfs_port)

add_library(lib_fatfs_stm32_usb_msc INTERFACE)
target_link_libraries(lib_fatfs_stm32_usb_msc INTERFACE fatfs_core fatfs_stm32_usb_msc_port)

add_library(lib_stm32_usb_device_cdc INTERFACE)
target_link_libraries(lib_stm32_usb_device_cdc INTERFACE
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_cdc_port)

add_library(lib_stm32_usb_device_audio INTERFACE)
target_link_libraries(lib_stm32_usb_device_audio INTERFACE
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_audio_port)

add_library(lib_stm32_usb_device_msc INTERFACE)
target_link_libraries(lib_stm32_usb_device_msc INTERFACE
    stm32_usb_device stm32_usb_device_conf_port stm32_usb_device_msc_port)

add_library(lib_stm32_usb_host_hid INTERFACE)
target_link_libraries(lib_stm32_usb_host_hid INTERFACE
    stm32_usb_host stm32_usb_host_conf_port)

add_library(lib_stm32_usb_host_msc INTERFACE)
target_link_libraries(lib_stm32_usb_host_msc INTERFACE
    stm32_usb_host stm32_usb_host_conf_port stm32_usb_host_msc_port)

add_library(lib_dsp INTERFACE)
target_link_libraries(lib_dsp INTERFACE cmsis_dsp)

set(LIB_ALL_TARGET
    lib_fatfs lib_fatfs_stm32_usb_msc
    lib_stm32_usb_device_cdc lib_stm32_usb_device_audio lib_stm32_usb_device_msc
    lib_stm32_usb_host_hid lib_stm32_usb_host_msc
    lib_dsp)

set(LIB_DIRS usmart malloc text picture audio mjpeg)
foreach(_d ${LIB_DIRS})
    add_subdirectory(${_d})
    list(APPEND LIB_ALL_TARGET lib_${_d})
endforeach()

set(LIB_ALL_TARGET ${LIB_ALL_TARGET} CACHE INTERNAL "" FORCE)

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
