set(LIB_SRC_USMART
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_str.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_port.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usmart_config.c CACHE INTERNAL "")
set(LIB_SRC_USMART_HAL ${HAL_SRC_TIM} CACHE INTERNAL "")
set(LIB_SRC_USMART_BSP ${BSP_SRC_LCD} ${BSP_SRC_SDRAM} CACHE INTERNAL "")

add_library(lib_usmart STATIC ${LIB_SRC_USMART})
set_target_properties(lib_usmart PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_usmart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_usmart PUBLIC bsp ${TARGET_LIB})

set(LIB_COMPONENTS ${LIB_COMPONENTS} USMART CACHE INTERNAL "" FORCE)
