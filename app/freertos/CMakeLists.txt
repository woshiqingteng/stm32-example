# FreeRTOS application tree.
# Default build type is debug (see tools/build.sh); each app has its own build tree.

set(FREERTOS_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(FREERTOS_HAL_COMMON
    ${HAL_SRC_CORE} ${HAL_SRC_CORTEX} ${HAL_SRC_GPIO}
    ${HAL_SRC_RCC}  ${HAL_SRC_PWR}    ${HAL_SRC_FLASH}
    ${HAL_SRC_UART} ${HAL_SRC_DMA})
set(FREERTOS_BSP_COMMON
    ${BSP_SRC_BSP} ${BSP_SRC_SYS} ${BSP_SRC_DELAY}
    ${BSP_SRC_USART} ${BSP_SRC_LED} ${BSP_SRC_KEY})

# Apps list only BSP and LIB components; the matching HAL sources are derived.
# Linker script selection is owned by the target layer (MCU_LD_SCRIPT property).
function(add_freertos_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;BSP;LIB" ${ARGN})
    set(_target app_freertos_${name})
    add_executable(${_target} ${ARG_SOURCES})

    bsp_resolve_hal(_hal ${ARG_BSP})
    lib_resolve(_libs _lib_hal _lib_bsp ${ARG_LIB})

    target_include_directories(freertos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FREERTOS_APP_DIR})
    target_compile_definitions(bsp PUBLIC USE_FREERTOS=1)
    target_link_libraries(bsp PUBLIC freertos)
    target_link_libraries(${_target} PRIVATE bsp freertos ${_libs})

    target_sources(freertos       PRIVATE ${FREERTOS_SRC})
    target_sources(bsp            PRIVATE ${FREERTOS_BSP_COMMON} ${ARG_BSP} ${_lib_bsp})
    target_sources(stm32f4xx_hal  PRIVATE ${FREERTOS_HAL_COMMON} ${_hal} ${_lib_hal})

    mcu_generate(${_target} ${name})
    mcu_flash(${_target} ${name})
endfunction()

add_subdirectory(${APP_NAME})
