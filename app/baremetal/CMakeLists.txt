# Baremetal app tree: common modules, target build, artifacts and flashing.

include(${CMAKE_SOURCE_DIR}/cmake/func/flow.cmake)

# Common module sources shared by every baremetal app (expanded lists).
set(BAREMETAL_COMMON
    ${TARGET_STM32F4}
    ${HAL_CORE} ${HAL_DMA} ${HAL_UART}
    ${BSP_CORE})

# app_baremetal_add(NAME [HAL <var>...] [BSP <var>...] <project file>...)
#   Order: project name, HAL modules, BSP modules, then the project's own files
#   (relative, e.g. main.c). The shared sources above are added here.
function(app_baremetal_add NAME)
    set(_srcs "")
    set(_mods "")

    foreach(_a ${ARGN})
        if(IS_ABSOLUTE "${_a}")
            list(APPEND _mods "${_a}")
        else()
            list(APPEND _srcs "${CMAKE_CURRENT_SOURCE_DIR}/${_a}")
        endif()
    endforeach()

    flow_firmware(app_baremetal_${NAME} SOURCES ${_srcs} ${BAREMETAL_COMMON} ${_mods})
endfunction()

foreach(_app ${APPS})
    add_subdirectory(${_app})
endforeach()
