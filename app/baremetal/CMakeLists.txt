# Baremetal application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

# Apps list SOURCES and (optionally) LIB components; all board drivers live in
# the bsp library and the linker keeps only the objects the image references.
# LIB defaults to empty (no library).
function(add_baremetal_app name)
    cmake_parse_arguments(APP "" "" "SOURCES;LIB" ${ARGN})

    if(NOT name)
        message(FATAL_ERROR "add_baremetal_app: <name> is required")
    endif()
    if(NOT APP_SOURCES)
        message(FATAL_ERROR "add_baremetal_app(${name}): SOURCES is required")
    endif()

    set(_target app_baremetal_${name})
    add_executable(${_target} ${APP_SOURCES})

    lib_resolve(_libs ${APP_LIB})

    target_link_libraries(${_target} PRIVATE bsp ${_libs})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
