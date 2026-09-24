# FreeRTOS application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

set(FREERTOS_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Apps list SOURCES and (optionally) LIB components. The FreeRTOS kernel is
# built by module/freertos; this helper supplies the app include path so the
# kernel compiles against the app's FreeRTOSConfig.h.
function(add_freertos_app name)
    cmake_parse_arguments(APP "" "" "SOURCES;LIB" ${ARGN})

    if(NOT name)
        message(FATAL_ERROR "add_freertos_app: <name> is required")
    endif()
    if(NOT APP_SOURCES)
        message(FATAL_ERROR "add_freertos_app(${name}): SOURCES is required")
    endif()

    set(_target app_freertos_${name})
    add_executable(${_target} ${APP_SOURCES})

    lib_resolve(_libs ${APP_LIB})

    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_APP_DIR})
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
    target_link_libraries(${_target} PRIVATE bsp freertos ${_libs})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
