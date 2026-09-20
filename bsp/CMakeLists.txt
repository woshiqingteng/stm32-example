# BSP tile selection via use_bsp(<tile>...).

# Append bsp.c plus the requested tiles to the calling app.
macro(use_bsp)
    list(APPEND APP_INCLUDES "${CMAKE_SOURCE_DIR}")
    list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/bsp.c")
    list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/syscalls.c")
    foreach(_tile ${ARGV})
        list(APPEND APP_SOURCES "${CMAKE_SOURCE_DIR}/bsp/${_tile}.c")
    endforeach()
endmacro()
