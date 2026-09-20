# Generate a disassembly listing without shell redirection.
execute_process(COMMAND ${OBJDUMP} -h -S ${INPUT} OUTPUT_FILE ${OUTPUT})
