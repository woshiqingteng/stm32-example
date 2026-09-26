# Port layer: vendor-stack -> hardware adaptation.
# Layout groups by nature first, then by feature:
#   common/      board-independent adapters
#   ${BSP}/      board-specific adapters
# The root only descends; each directory declares its own targets.

add_subdirectory(${BSP})
add_subdirectory(common)
