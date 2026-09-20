# Generic compile/link/artifact/flash for one firmware target.
# Module/source selection is done by the caller; this file only compiles.

find_program(OPENOCD_EXECUTABLE openocd)
add_custom_target(flash)

# flow_compile(TARGET SOURCES <file>... [OUTDIR <dir>] [DEFS <def>...])
function(flow_compile TARGET)
    set(_srcs "")
    set(_outdir "${CMAKE_BINARY_DIR}/${TARGET}")
    set(_defs "")
    set(_kw "")

    foreach(_a ${ARGN})
        if(_a STREQUAL "SOURCES" OR _a STREQUAL "OUTDIR" OR _a STREQUAL "DEFS")
            set(_kw "${_a}")
        elseif(_kw STREQUAL "SOURCES")
            list(APPEND _srcs "${_a}")
        elseif(_kw STREQUAL "OUTDIR")
            set(_outdir "${_a}")
        elseif(_kw STREQUAL "DEFS")
            list(APPEND _defs "${_a}")
        endif()
    endforeach()

    add_executable(${TARGET} ${_srcs})
    target_include_directories(${TARGET} PRIVATE
        "${STM32_TARGET_DIR}" "${BSP_DIR}" "${CMSIS_CORE_INCLUDE}" "${HAL_INC}")
    target_compile_definitions(${TARGET} PRIVATE
        "USE_HAL_DRIVER" "${STM32_DEVICE_MACRO}" "HSE_VALUE=${STM32_HSE_VALUE}" ${_defs})
    if(HAL_SOURCES)
        set_source_files_properties(${HAL_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
    endif()

    target_link_options(${TARGET} PRIVATE -T${STM32_LINKER_SCRIPT}
        -Wl,-Map=$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.map)
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${_outdir}"
        OUTPUT_NAME "${TARGET}")

    set(_dir "$<TARGET_FILE_DIR:${TARGET}>")
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET}> ${_dir}/${TARGET}.bin)
    if(MCU_GEN_FULL_ARTIFACT)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET}> ${_dir}/${TARGET}.hex)
    endif()

    add_custom_target(flash_${TARGET}
        COMMAND ${OPENOCD_EXECUTABLE} -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg
                -c "program ${_dir}/${TARGET}.bin 0x08000000 verify reset exit"
        DEPENDS ${TARGET}
        VERBATIM)
    add_dependencies(flash flash_${TARGET})
endfunction()
