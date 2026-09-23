# USMART serial shell. All sources in this directory build the lib_usmart target.
file(GLOB _src CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(lib_usmart STATIC ${_src})
set_target_properties(lib_usmart PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_include_directories(lib_usmart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_usmart PUBLIC bsp)

set(LIB_SRC_USMART USMART CACHE INTERNAL "")
set(LIB_COMPONENTS ${LIB_COMPONENTS} USMART CACHE INTERNAL "" FORCE)
