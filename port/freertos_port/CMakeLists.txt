# Port layer: common FreeRTOS configuration shared by every FreeRTOS app
# (FreeRTOSConfig_common.h). Each app keeps its own FreeRTOSConfig.h in the app
# directory and includes this one.
add_library(freertos_port INTERFACE)
target_include_directories(freertos_port INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET freertos)
    target_link_libraries(freertos PRIVATE freertos_port)
endif()
