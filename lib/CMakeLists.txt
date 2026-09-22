# Library layer: reusable application middleware. Each library is an
# independent target (lib_<name>) selected by apps via LIB components.

add_subdirectory(usmart)

# lib_resolve(<libs-out> <hal-out> <bsp-out> <lib sources...>): map the given
# LIB source lists to their library targets plus the HAL/BSP sources the
# matched components require.
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
            string(TOLOWER ${_comp} _lc)
            list(APPEND _libs lib_${_lc})
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
