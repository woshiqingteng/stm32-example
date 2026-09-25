# ============================================================
# 1. 直连组件：用 INTERFACE 包成 lib_*，对外名字统一
# ============================================================
add_library(lib_fatfs INTERFACE)
target_link_libraries(lib_fatfs INTERFACE fatfs_core fatfs_port)

add_library(lib_stm32_usb_device_cdc INTERFACE)
target_link_libraries(lib_stm32_usb_device_cdc INTERFACE
    stm32_usb_device stm32_usb_device_common_port stm32_usb_device_cdc_port)

add_library(lib_stm32_usb_device_audio INTERFACE)
target_link_libraries(lib_stm32_usb_device_audio INTERFACE
    stm32_usb_device stm32_usb_device_common_port stm32_usb_device_audio_port)

add_library(lib_stm32_usb_device_msc INTERFACE)
target_link_libraries(lib_stm32_usb_device_msc INTERFACE
    stm32_usb_device stm32_usb_device_common_port stm32_usb_device_msc_port)

add_library(lib_stm32_usb_host_hid INTERFACE)
target_link_libraries(lib_stm32_usb_host_hid INTERFACE
    stm32_usb_host stm32_usb_host_common_port)

add_library(lib_stm32_usb_host_msc INTERFACE)
target_link_libraries(lib_stm32_usb_host_msc INTERFACE
    stm32_usb_host stm32_usb_host_common_port stm32_usb_host_msc_port)

add_library(lib_dsp INTERFACE)
target_link_libraries(lib_dsp INTERFACE cmsis_dsp)

# ============================================================
# 2. 中间件库：目录名 = 目标名（去掉 lib_ 前缀），一份清单驱动两件事
# ============================================================
set(LIB_ALL_TARGETS
    lib_fatfs
    lib_stm32_usb_device_cdc lib_stm32_usb_device_audio lib_stm32_usb_device_msc
    lib_stm32_usb_host_hid lib_stm32_usb_host_msc
    lib_dsp)

set(LIB_DIRS usmart malloc text picture audio mjpeg)
foreach(_d ${LIB_DIRS})
    add_subdirectory(${_d})              # 加子目录
    list(APPEND LIB_ALL_TARGETS lib_${_d})   # 同时进 ALL
endforeach()

# lib_resolve() 在 app 目录调用，LIB_ALL_TARGETS 必须是全局可见的缓存变量。
set(LIB_ALL_TARGETS ${LIB_ALL_TARGETS} CACHE INTERNAL "" FORCE)

# ============================================================
# 3. 解析函数：XXXX -> lib_xxxx；ALL -> 全部
# ============================================================
function(lib_resolve out)
    # ALL 必须单独使用
    if("ALL" IN_LIST ARGN)
        list(LENGTH ARGN _n)
        if(_n GREATER 1)
            message(FATAL_ERROR "LIB: 'ALL' must be used alone")
        endif()
        set(${out} ${LIB_ALL_TARGETS} PARENT_SCOPE)
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
