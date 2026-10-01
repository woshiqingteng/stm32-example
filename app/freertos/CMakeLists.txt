function(add_freertos_app name)
    cmake_parse_arguments(APP "" "" "SOURCE;LIB;BSP" ${ARGN})

    # check
    if(NOT name)
        message(FATAL_ERROR "add_freertos_app: <name> is required")
    endif()
    if(NOT APP_SOURCE)
        message(FATAL_ERROR "add_freertos_app(${name}): SOURCE is required")
    endif()

    set(_target app_freertos_${name})
    add_executable(${_target} ${APP_SOURCE})
    target_compile_definitions(${_target} PRIVATE APP_BANNER="${_target}")

    # parse lib
    lib_resolve(_libs ${APP_LIB})
    bsp_resolve(_bsp ${APP_BSP})
    message(STATUS "[${name}] SOURCE = ${APP_SOURCE}")
    message(STATUS "[${name}] BSP    = ${APP_BSP}")
    message(STATUS "[${name}] LIB    = ${APP_LIB}")

    # link
    # The app dir provides FreeRTOSConfig.h; PUBLIC because every target that
    # includes FreeRTOS.h (port, bsp_delay, eth, the app) must see the config.
    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${_target} PRIVATE ${_bsp} lib_freertos ${_libs})

    # generate
    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
