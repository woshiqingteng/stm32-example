function(add_baremetal_app name)
    cmake_parse_arguments(APP "" "" "SOURCE;LIB;BSP" ${ARGN})

    # check
    if(NOT name)
        message(FATAL_ERROR "add_baremetal_app: <name> is required")
    endif()
    if(NOT APP_SOURCE)
        message(FATAL_ERROR "add_baremetal_app(${name}): SOURCE is required")
    endif()

    set(_target app_baremetal_${name})
    add_executable(${_target} ${APP_SOURCE})

    # parse lib
    lib_resolve(_libs ${APP_LIB})
    bsp_resolve(_bsp ${APP_BSP})
    message(STATUS "[${name}] SOURCE = ${APP_SOURCE}")
    message(STATUS "[${name}] BSP     = ${APP_BSP}")
    message(STATUS "[${name}] LIB     = ${APP_LIB}")

    # link
    target_link_libraries(${_target} PRIVATE ${_bsp} ${_libs})

    # generate
    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
