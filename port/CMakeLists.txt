# Port layer: vendor-stack -> hardware adaptation.
# Each board keeps its own directory; the root only descends.

if(BSP STREQUAL "openedv_stm32f4")
    add_subdirectory(openedv_stm32f4)
endif()
