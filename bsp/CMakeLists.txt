# Board support package layer: dispatch to the board selected by BOARD.
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${BSP_BOARD_DIR})
    message(FATAL_ERROR "No BSP for board '${BOARD}': bsp/${BSP_BOARD_DIR}")
endif()
add_subdirectory(${BSP_BOARD_DIR})
