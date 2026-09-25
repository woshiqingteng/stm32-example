# Baremetal application tree.
# Default build type is debug (see tool/build.sh); each app has its own build tree.

# Apps list SOURCES and (optionally) LIB components and the BSP drivers they
# use directly; bsp_core is always linked. HAL is pulled in implicitly by the
# BSP drivers. LIB/BSP default to empty.
function(add_baremetal_app name)
    cmake_parse_arguments(APP "" "" "SOURCES;LIB;BSP" ${ARGN})

    if(NOT name)
        message(FATAL_ERROR "add_baremetal_app: <name> is required")
    endif()
    if(NOT APP_SOURCES)
        message(FATAL_ERROR "add_baremetal_app(${name}): SOURCES is required")
    endif()

    set(_target app_baremetal_${name})
    add_executable(${_target} ${APP_SOURCES})

    lib_resolve(_libs ${APP_LIB})
    bsp_resolve(_bsp ${APP_BSP})

    target_link_libraries(${_target} PRIVATE ${_bsp} ${_libs})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
