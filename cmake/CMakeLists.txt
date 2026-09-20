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

# Append items to a CACHE INTERNAL list variable.
macro(cache_list_append _var)
    set(_cache_tmp "${${_var}}")
    list(APPEND _cache_tmp ${ARGN})
    set(${_var} "${_cache_tmp}" CACHE INTERNAL "" FORCE)
endmacro()

# Start a new application: reset the CACHE INTERNAL accumulators.
macro(app_begin NAME)
    set(APP_NAME "${NAME}" CACHE INTERNAL "" FORCE)
    set(APP_SOURCES "" CACHE INTERNAL "" FORCE)
    set(APP_INCLUDES "" CACHE INTERNAL "" FORCE)
    set(APP_SYSTEM_INCLUDES "" CACHE INTERNAL "" FORCE)
    set(APP_DEFINES "" CACHE INTERNAL "" FORCE)
    set(APP_DEPS "" CACHE INTERNAL "" FORCE)
    set(_CMSIS_ADDED FALSE CACHE INTERNAL "" FORCE)
    set(_TARGET_ADDED FALSE CACHE INTERNAL "" FORCE)
    set(_HAL_CORE_ADDED FALSE CACHE INTERNAL "" FORCE)
    set(_HAL_MODULES "" CACHE INTERNAL "" FORCE)
    set(_BSP_CORE_ADDED FALSE CACHE INTERNAL "" FORCE)
    set(_BSP_TILES "" CACHE INTERNAL "" FORCE)
endmacro()

# Append link dependencies; parameters are appended to APP_DEPS.
macro(use_dep)
    cache_list_append(APP_DEPS ${ARGV})
endmacro()

# Create app_<os>_<name> from the accumulators, then clear them.
macro(app_add)
    set(_tgt "app_${OS}_${APP_NAME}")
    add_executable(${_tgt} ${APP_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
    target_include_directories(${_tgt} PRIVATE ${APP_INCLUDES})
    target_include_directories(${_tgt} SYSTEM PRIVATE ${APP_SYSTEM_INCLUDES})
    target_compile_definitions(${_tgt} PRIVATE ${APP_DEFINES})
    if(APP_DEPS)
        target_link_libraries(${_tgt} PRIVATE ${APP_DEPS})
    endif()
    target_link_options(${_tgt} PRIVATE
        -T${TARGET_LD_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.map)
    set_target_properties(${_tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OS}/${APP_NAME}"
        OUTPUT_NAME "${_tgt}")
    flow_add_artifacts(${_tgt})
    flow_add_flash(${APP_NAME} $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.bin ${_tgt})

    set(APP_NAME "" CACHE INTERNAL "" FORCE)
    set(APP_SOURCES "" CACHE INTERNAL "" FORCE)
    set(APP_INCLUDES "" CACHE INTERNAL "" FORCE)
    set(APP_SYSTEM_INCLUDES "" CACHE INTERNAL "" FORCE)
    set(APP_DEFINES "" CACHE INTERNAL "" FORCE)
    set(APP_DEPS "" CACHE INTERNAL "" FORCE)
endmacro()
