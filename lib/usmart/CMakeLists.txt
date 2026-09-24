# USMART serial shell.
set(USMART_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_str.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_config.c)

add_library(lib_usmart STATIC ${USMART_SRC})

target_include_directories(lib_usmart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_usmart PUBLIC bsp)
