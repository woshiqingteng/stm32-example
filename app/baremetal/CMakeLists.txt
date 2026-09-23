# Baremetal application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

# Apps list only LIB components; all board drivers live in the bsp library and
# the linker keeps only the objects the image references.
function(add_baremetal_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIB" ${ARGN})
    set(_target app_baremetal_${name})
    add_executable(${_target} ${ARG_SOURCES})

    lib_resolve(_libs ${ARG_LIB})

    target_link_libraries(${_target} PRIVATE bsp ${_libs})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
