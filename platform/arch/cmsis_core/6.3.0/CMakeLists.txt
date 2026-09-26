add_library(cmsis_core INTERFACE)

# series
if(MCU_ARCH MATCHES "^cortex-([mra])")
    set(CMSIS_PROFILE ${CMAKE_MATCH_1}-profile)
else()
    message(FATAL_ERROR "Unsupported Cortex series: ${MCU_ARCH}")
endif()

# number
string(REGEX REPLACE "^cortex-" "" CMSIS_CORE ${MCU_ARCH})

set(CMSIS_CORES
    m0 m0plus m1 m3 m4 m7 m23 m33 m35p m52 m55 m85
    r4 r5 r7 r8 r52
    a5 a7 a9 a35 a53 a55 a57)
if(NOT CMSIS_CORE IN_LIST CMSIS_CORES)
    message(FATAL_ERROR "Unsupported Cortex core: ${MCU_ARCH}")
endif()
message(STATUS "CMSIS-Core profile=${CMSIS_PROFILE} core=${CMSIS_CORE}")

target_include_directories(cmsis_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
