# Board / CPU / function selection and shared app macros.

set(TOOLCHAIN "gcc" CACHE STRING "Toolchain: gcc")
set(BOARD "openedv_stm32f4" CACHE STRING "Board")
set(OS "baremetal" CACHE STRING "OS: baremetal or freertos")

set(MCU_GEN_FULL_ARTIFACT OFF CACHE BOOL "Also generate .hex and .lst")

include(${CMAKE_CURRENT_LIST_DIR}/board/${BOARD}.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cpu/${CPU}.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/func/flow.cmake)

if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
endif()
if(NOT CMAKE_OBJDUMP)
    find_program(CMAKE_OBJDUMP arm-none-eabi-objdump REQUIRED)
endif()

# Begin defining an application target (accumulators in the caller's directory scope).
macro(app_begin NAME)
    set(APP_NAME "${NAME}")
    set(APP_SOURCES "")
    set(APP_INCLUDES "")
    set(APP_SYSTEM_INCLUDES "")
    set(APP_DEFINES "")
    set(_CMSIS_ADDED FALSE)
    set(_TARGET_ADDED FALSE)
    set(_HAL_CORE_ADDED FALSE)
    set(_HAL_MODULES "")
    set(_BSP_CORE_ADDED FALSE)
    set(_BSP_TILES "")
endmacro()

# Create app_<os>_<name> from the accumulated sources and emit the .bin.
macro(app_add)
    set(_tgt "app_${OS}_${APP_NAME}")
    add_executable(${_tgt} ${APP_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
    target_include_directories(${_tgt} PRIVATE ${APP_INCLUDES})
    target_include_directories(${_tgt} SYSTEM PRIVATE ${APP_SYSTEM_INCLUDES})
    target_compile_definitions(${_tgt} PRIVATE ${APP_DEFINES})
    target_link_options(${_tgt} PRIVATE
        -T${TARGET_LD_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.map)
    set_target_properties(${_tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OS}/${APP_NAME}"
        OUTPUT_NAME "${_tgt}")
    add_custom_command(TARGET ${_tgt} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${_tgt}>
                $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.bin
        COMMENT "objcopy: ${_tgt}.bin")
    if(MCU_GEN_FULL_ARTIFACT)
        add_custom_command(TARGET ${_tgt} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${_tgt}>
                    $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.hex
            COMMAND ${CMAKE_COMMAND}
                    -DOBJDUMP=${CMAKE_OBJDUMP}
                    -DINPUT=$<TARGET_FILE:${_tgt}>
                    -DOUTPUT=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.lst
                    -P ${CMAKE_SOURCE_DIR}/cmake/scripts/gen_lst.cmake
            COMMENT "objcopy: ${_tgt}.hex/.lst")
    endif()
    flow_add_flash(${APP_NAME} $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.bin ${_tgt})
endmacro()
