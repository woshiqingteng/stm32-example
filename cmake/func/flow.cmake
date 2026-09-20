# Baremetal app flow: resolve module tokens -> link -> artifacts -> flash.

find_program(OPENOCD_EXECUTABLE openocd)

# Inline listing helper for MCU_GEN_FULL_ARTIFACT (.lst), no shell redirection.
set(FLOW_GEN_LST "${CMAKE_BINARY_DIR}/gen_lst.cmake")
file(WRITE "${FLOW_GEN_LST}"
     "execute_process(COMMAND \${OBJDUMP} -h -S \${INPUT} OUTPUT_FILE \${OUTPUT})\n")

add_custom_target(flash)

# flow_app_baremetal(NAME SOURCES <file>... [MODULES <token>...])
function(flow_app_baremetal NAME)
    set(_srcs "")
    set(_mods "")
    set(_kw "")

    foreach(_a ${ARGN})
        if(_a STREQUAL "SOURCES")
            set(_kw SOURCES)
        elseif(_a STREQUAL "MODULES")
            set(_kw MODULES)
        elseif(_kw STREQUAL "SOURCES")
            list(APPEND _srcs "${CMAKE_CURRENT_SOURCE_DIR}/${_a}")
        elseif(_kw STREQUAL "MODULES")
            list(APPEND _mods "${_a}")
        endif()
    endforeach()

    foreach(_m ${BAREMETAL_COMMON} ${_mods})
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

    target_link_options(${_tgt} PRIVATE -T${STM32_LINKER_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${_tgt}>/${_tgt}.map)
    set_target_properties(${_tgt} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OS}/${NAME}"
        OUTPUT_NAME "${_tgt}")

    set(_dir "$<TARGET_FILE_DIR:${_tgt}>")
    add_custom_command(TARGET ${_tgt} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${_tgt}> ${_dir}/${_tgt}.bin)
    if(MCU_GEN_FULL_ARTIFACT)
        add_custom_command(TARGET ${_tgt} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${_tgt}> ${_dir}/${_tgt}.hex
            COMMAND ${CMAKE_COMMAND} -DOBJDUMP=${CMAKE_OBJDUMP}
                    -DINPUT=$<TARGET_FILE:${_tgt}> -DOUTPUT=${_dir}/${_tgt}.lst
                    -P ${FLOW_GEN_LST})
    endif()

    add_custom_target(flash_${NAME}
        COMMAND ${OPENOCD_EXECUTABLE} -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg
                -c "program ${_dir}/${_tgt}.bin 0x08000000 verify reset exit"
        DEPENDS ${_tgt}
        VERBATIM)
    add_dependencies(flash flash_${NAME})
endfunction()
