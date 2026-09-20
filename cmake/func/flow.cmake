# Link / artifact / flash helpers and the complete baremetal app flow.

find_program(OPENOCD_EXECUTABLE openocd)

set(OPENOCD_INTERFACE  "interface/cmsis-dap.cfg" CACHE STRING "openocd interface config")
set(OPENOCD_TARGET_CFG "target/stm32f4x.cfg"     CACHE STRING "openocd target config")
set(FLASH_ADDRESS      "0x08000000"              CACHE STRING "Flash base address")

# Inline listing helper (kept here instead of a separate source file).
set(FLOW_GEN_LST_SCRIPT "${CMAKE_BINARY_DIR}/gen_lst.cmake")
file(WRITE "${FLOW_GEN_LST_SCRIPT}"
     "execute_process(COMMAND \${OBJDUMP} -h -S \${INPUT} OUTPUT_FILE \${OUTPUT})\n")

add_custom_target(flash)

# Always produce .bin; also .hex/.lst when MCU_GEN_FULL_ARTIFACT is ON.
function(flow_add_artifacts tgt)
    add_custom_command(TARGET ${tgt} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${tgt}>
                $<TARGET_FILE_DIR:${tgt}>/${tgt}.bin
        COMMENT "objcopy: ${tgt}.bin")

    if(MCU_GEN_FULL_ARTIFACT)
        add_custom_command(TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${tgt}>
                    $<TARGET_FILE_DIR:${tgt}>/${tgt}.hex
            COMMAND ${CMAKE_COMMAND}
                    -DOBJDUMP=${CMAKE_OBJDUMP}
                    -DINPUT=$<TARGET_FILE:${tgt}>
                    -DOUTPUT=$<TARGET_FILE_DIR:${tgt}>/${tgt}.lst
                    -P ${FLOW_GEN_LST_SCRIPT}
            COMMENT "objcopy: ${tgt}.hex/.lst")
    endif()
endfunction()

function(flow_add_flash name bin tgt)
    if(NOT OPENOCD_EXECUTABLE)
        return()
    endif()
    add_custom_target(flash_${name}
        COMMAND ${OPENOCD_EXECUTABLE} -f ${OPENOCD_INTERFACE} -f ${OPENOCD_TARGET_CFG}
                -c "program ${bin} ${FLASH_ADDRESS} verify reset exit"
        DEPENDS ${tgt}
        COMMENT "Flashing ${tgt}"
        VERBATIM)
    add_dependencies(flash flash_${name})
endfunction()

if(OPENOCD_EXECUTABLE)
    add_custom_target(erase
        COMMAND ${OPENOCD_EXECUTABLE} -f ${OPENOCD_INTERFACE} -f ${OPENOCD_TARGET_CFG}
                -c "init; reset halt; stm32f4x mass_erase 0; exit"
        COMMENT "Erasing flash"
        VERBATIM)
    add_custom_target(reset
        COMMAND ${OPENOCD_EXECUTABLE} -f ${OPENOCD_INTERFACE} -f ${OPENOCD_TARGET_CFG}
                -c "init; reset run; exit"
        COMMENT "Resetting target"
        VERBATIM)
endif()

# Complete application flow: resolve module tokens to sources, create the target,
# link it, and wire artifacts + flashing.
# flow_app_baremetal(NAME SOURCES <file>... [MODULES <token>...])
function(flow_app_baremetal NAME)
    set(_sources "")
    set(_modules "")
    set(_kw "")

    foreach(_a ${ARGN})
        if(_a STREQUAL "SOURCES" OR _a STREQUAL "MODULES")
            set(_kw "${_a}")
        elseif(_kw STREQUAL "SOURCES")
            list(APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${_a}")
        elseif(_kw STREQUAL "MODULES")
            list(APPEND _modules "${_a}")
        endif()
    endforeach()

    set(_srcs ${_sources})
    foreach(_m ${BAREMETAL_COMMON} ${_modules})
        list(APPEND _srcs ${${_m}})
    endforeach()

    set(_tgt "app_${OS}_${NAME}")
    add_executable(${_tgt} ${_srcs})
    target_include_directories(${_tgt} PRIVATE
        "${STM32_TARGET_DIR}" "${BSP_DIR}" "${CMSIS_CORE_INCLUDE}" "${HAL_INC}")
    target_compile_definitions(${_tgt} PRIVATE
        "USE_HAL_DRIVER" "${STM32_DEVICE_MACRO}" "HSE_VALUE=${STM32_HSE_VALUE}"
        "BSP_SUPPORT_OS=${BSP_SUPPORT_OS}")

    if(HAL_SOURCES)
        set_source_files_properties(${HAL_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
    endif()

    target_link_options(${_tgt} PRIVATE
        -T${STM32_LINKER_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.map)
    set_target_properties(${_tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OS}/${NAME}"
        OUTPUT_NAME "${_tgt}")

    flow_add_artifacts(${_tgt})
    flow_add_flash(${NAME} $<TARGET_FILE_DIR:${_tgt}>/${_tgt}.bin ${_tgt})
endfunction()
