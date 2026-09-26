set(FREERTOS_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(add_freertos_app name)
    cmake_parse_arguments(APP "" "" "SOURCES;LIB;BSP" ${ARGN})

    # check
    if(NOT name)
        message(FATAL_ERROR "add_freertos_app: <name> is required")
    endif()
    if(NOT APP_SOURCES)
        message(FATAL_ERROR "add_freertos_app(${name}): SOURCES is required")
    endif()

    set(_target app_freertos_${name})
    add_executable(${_target} ${APP_SOURCES})

    # parse lib
    lib_resolve(_libs ${APP_LIB})
    bsp_resolve(_bsp ${APP_BSP})
    message(STATUS "[${name}] SOURCES = ${APP_SOURCES}")
    message(STATUS "[${name}] BSP     = ${APP_BSP}")
    message(STATUS "[${name}] LIB     = ${APP_LIB}")

    # link
    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_APP_DIR})
    target_link_libraries(${_target} PRIVATE ${_bsp} freertos ${_libs})

    # generate
    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
