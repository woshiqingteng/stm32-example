# Link/flash helpers (openocd + cmsis-dap).

set(OPENOCD_EXECUTABLE "D:/app/openocd/v0.12.0/i686-w64-mingw32/bin/openocd.exe"
    CACHE FILEPATH "openocd executable")
if(NOT EXISTS "${OPENOCD_EXECUTABLE}")
    find_program(OPENOCD_EXECUTABLE openocd)
endif()

set(OPENOCD_INTERFACE "interface/cmsis-dap.cfg" CACHE STRING "openocd interface config")
set(OPENOCD_TARGET_CFG "target/stm32f4x.cfg" CACHE STRING "openocd target config")
set(FLASH_ADDRESS "0x08000000" CACHE STRING "Flash base address")

add_custom_target(flash)

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
