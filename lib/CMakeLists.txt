# Library layer: reusable application middleware. Each lib_<name> target is
# built from all .c files in its own directory. Apps select libraries through
# the LIB component tokens below; the linker keeps only referenced objects.
# Dependency rule: lib may depend on module + bsp + port.

set(LIB_COMPONENTS "" CACHE INTERNAL "")

# Components that map straight onto port/module targets (no lib_<name> wrapper).
set(LIB_SRC_FATFS              FATFS      CACHE INTERNAL "")
set(LIB_SRC_FATFS_TARGETS      fatfs_core fatfs_port CACHE INTERNAL "")
set(LIB_SRC_USB_DEVICE         USB_DEVICE      CACHE INTERNAL "")
set(LIB_SRC_USB_DEVICE_TARGETS usb_device usb_port CACHE INTERNAL "")
set(LIB_SRC_USB_HOST           USB_HOST      CACHE INTERNAL "")
set(LIB_SRC_USB_HOST_TARGETS   usb_host usb_port CACHE INTERNAL "")
set(LIB_SRC_DSP                DSP       CACHE INTERNAL "")
set(LIB_SRC_DSP_TARGETS        cmsis_dsp m CACHE INTERNAL "")

set(LIB_COMPONENTS ${LIB_COMPONENTS} FATFS USB_DEVICE USB_HOST DSP CACHE INTERNAL "" FORCE)

add_subdirectory(usmart)
add_subdirectory(malloc)
add_subdirectory(text)
add_subdirectory(picture)
add_subdirectory(audio)
add_subdirectory(mjpeg)

# lib_resolve(<libs-out> <lib tokens...>): map the selected LIB component tokens
# to their library/port/module targets.
function(lib_resolve libs_var)
    set(_libs "")
    foreach(_comp ${LIB_COMPONENTS})
        if(LIB_SRC_${_comp} IN_LIST ARGN)
            if(DEFINED LIB_SRC_${_comp}_TARGETS)
                list(APPEND _libs ${LIB_SRC_${_comp}_TARGETS})
            else()
                string(TOLOWER ${_comp} _lc)
                list(APPEND _libs lib_${_lc})
            endif()
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _libs)
    set(${libs_var} ${_libs} PARENT_SCOPE)
endfunction()
