# Board support layer: dispatch to the board selected by BOARD.
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${BSP})
    message(FATAL_ERROR "No BSP for board '${BOARD}': bsp/${BSP}")
endif()
add_subdirectory(${BSP})
