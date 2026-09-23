# FreeRTOS application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

set(FREERTOS_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Apps list only LIB components; all board drivers live in the bsp library and
# the linker keeps only the objects the image references.
function(add_freertos_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIB" ${ARGN})
    set(_target app_freertos_${name})
    add_executable(${_target} ${ARG_SOURCES})

    lib_resolve(_libs ${ARG_LIB})

    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_APP_DIR})
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
    target_link_libraries(${_target} PRIVATE bsp freertos ${_libs})

    target_sources(freertos PRIVATE ${FREERTOS_SRC})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
