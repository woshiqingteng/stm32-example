set(LWIP_MQTT_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip_mqtt_common.c)

add_library(lib_lwip_mqtt_common STATIC EXCLUDE_FROM_ALL ${LWIP_MQTT_COMMON_SRC})

target_include_directories(lib_lwip_mqtt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_lwip_mqtt_common PUBLIC lwip_port)
