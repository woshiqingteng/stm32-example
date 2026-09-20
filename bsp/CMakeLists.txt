# BSP selection via use_bsp(<tile>...).
# Idempotent: bsp.c + syscalls.c are added once, tiles are de-duplicated.

macro(use_bsp)
    if(NOT _BSP_CORE_ADDED)
        list(APPEND APP_INCLUDES "${CMAKE_SOURCE_DIR}/bsp")
        list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/bsp.c")
        list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/syscalls.c")
        set(_BSP_CORE_ADDED TRUE)
        set(_BSP_TILES "")
    endif()

    foreach(_tile ${ARGV})
        if(NOT _tile IN_LIST _BSP_TILES)
            list(APPEND _BSP_TILES ${_tile})
            list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/${_tile}.c")
        endif()
    endforeach()
endmacro()
