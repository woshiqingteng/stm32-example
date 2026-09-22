set(FREERTOS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/timers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F/port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/MemMang/heap_4.c
    CACHE INTERNAL "")

add_library(freertos STATIC)

target_include_directories(freertos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portable/GCC/ARM_CM4F)

target_link_libraries(freertos PRIVATE ${TARGET_LIB})
