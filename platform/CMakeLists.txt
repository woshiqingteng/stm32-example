# Platform layer: the non-optional base selected by MCU/board.
#   arch/  CPU architecture (Arm CMSIS-Core)
#   soc/   SoC device headers + system + startup + vector + it + linker scripts
#
# Both are added before every other layer so the whole build is ordered bottom-up
# (platform -> module -> bsp -> port -> lib -> app) with no forward references.

add_subdirectory(arch)
add_subdirectory(soc)
