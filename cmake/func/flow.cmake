set(MCU_GEN_FULL_ARTIFACT OFF CACHE BOOL "Generate full artifacts")

function(mcu_generate target base)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME "${base}.elf")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
                $<TARGET_FILE:${target}>
                $<TARGET_FILE_DIR:${target}>/${base}.bin
        COMMENT "Generating ${base}.bin")
    if(MCU_GEN_FULL_ARTIFACT)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex
                    $<TARGET_FILE:${target}>
                    $<TARGET_FILE_DIR:${target}>/${base}.hex
            COMMENT "Generating ${base}.hex")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -h -S $<TARGET_FILE:${target}>
                    > $<TARGET_FILE_DIR:${target}>/${base}.lst
            COMMENT "Generating ${base}.lst")
    endif()
endfunction()

function(mcu_flash target base)
    add_custom_target(flash
        COMMAND ${OPENOCD} -f interface/${OPENOCD_INTERFACE} -f target/${OPENOCD_TARGET}
                -c "program $<TARGET_FILE_DIR:${target}>/${base}.bin ${MCU_FLASH_BASE} verify reset exit"
        DEPENDS ${target}
        COMMENT "Flashing ${base}.bin via OpenOCD (CMSIS-DAP)")
endfunction()
