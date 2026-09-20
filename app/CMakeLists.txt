# Select the OS variant of the application tree.

if(OS STREQUAL "baremetal")
    add_subdirectory(baremetal)
elseif(OS STREQUAL "freertos")
    add_subdirectory(freertos)
else()
    message(FATAL_ERROR "Unknown OS: ${OS}")
endif()
