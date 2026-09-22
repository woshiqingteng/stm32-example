if(NOT BOARD)
    message(FATAL_ERROR "BOARD not set")
endif()
include(board/${BOARD}.cmake)
message(STATUS "Board     : ${BOARD}")

# Every board manifest must define the full chip/HAL/BSP selection so that the
# module, target and bsp layers can stay free of hard-coded board knowledge.
foreach(_v
        BOARD CPU STM32_SERIES STM32_PART STM32_DEVICE_MACRO
        HAL_MODULE_DIR HAL_MODULE_VERSION BSP_BOARD_DIR TARGET_LIB)
    if(NOT ${_v})
        message(FATAL_ERROR "BOARD '${BOARD}': manifest variable '${_v}' not set")
    endif()
endforeach()

if(APP_TARGET MATCHES "^app_([a-zA-Z0-9]+)_(.+)$")
    set(OS       ${CMAKE_MATCH_1} CACHE INTERNAL "" FORCE)
    set(APP_NAME ${CMAKE_MATCH_2} CACHE INTERNAL "" FORCE)
else()
    message(FATAL_ERROR "Invalid APP_TARGET: ${APP_TARGET} (expect app_<os>_<name>)")
endif()
message(STATUS "App       : ${APP_TARGET}  OS: ${OS}  name: ${APP_NAME}")

message(STATUS "HAL       : module/${HAL_MODULE_DIR}/${HAL_MODULE_VERSION}")
message(STATUS "BSP       : bsp/${BSP_BOARD_DIR}")
message(STATUS "Target    : target/${STM32_SERIES}")

include(${CMAKE_CURRENT_LIST_DIR}/func/flow.cmake)
