# Baremetal app tree: common modules live here; each project supplements its
# own sources/modules via app_baremetal_add. cmake/func/flow.cmake only compiles.

include(${CMAKE_SOURCE_DIR}/cmake/func/flow.cmake)

set(BAREMETAL_COMMON_HAL HAL_CORE HAL_UART)
set(BAREMETAL_COMMON_BSP BSP_CORE)
set(BAREMETAL_COMMON TARGET_STM32F4 ${BAREMETAL_COMMON_HAL} ${BAREMETAL_COMMON_BSP})

# app_baremetal_add(NAME SOURCES <file>... [MODULES <token>...])
function(app_baremetal_add NAME)
    set(_srcs "")
    set(_mods "")
    set(_kw "")

    foreach(_a ${ARGN})
        if(_a STREQUAL "SOURCES")
            set(_kw SOURCES)
        elseif(_a STREQUAL "MODULES")
            set(_kw MODULES)
        elseif(_kw STREQUAL "SOURCES")
            list(APPEND _srcs "${CMAKE_CURRENT_SOURCE_DIR}/${_a}")
        elseif(_kw STREQUAL "MODULES")
            list(APPEND _mods "${_a}")
        endif()
    endforeach()

    foreach(_m ${BAREMETAL_COMMON} ${_mods})
        list(APPEND _srcs ${${_m}})
    endforeach()

    flow_compile(app_baremetal_${NAME} SOURCES ${_srcs})
endfunction()

foreach(_app ${APPS})
    add_subdirectory(${_app})
endforeach()
