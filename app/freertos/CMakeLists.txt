# FreeRTOS application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

set(FREERTOS_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Apps list only BSP and LIB components; the matching BSP component sources are
# expanded transitively (bsp_expand). The DRV (HAL) is reached through bsp's
# public link and the linker keeps only the objects the image references.
function(add_freertos_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;BSP;LIB" ${ARGN})
    set(_target app_freertos_${name})
    add_executable(${_target} ${ARG_SOURCES})

    lib_resolve(_libs _lib_bsp ${ARG_LIB})
    bsp_expand(_bsp_full ${ARG_BSP} ${_lib_bsp})

    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_APP_DIR})
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
    target_link_libraries(${_target} PRIVATE bsp freertos ${_libs})

    target_sources(freertos PRIVATE ${FREERTOS_SRC})
    target_sources(bsp      PRIVATE ${_bsp_full})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
