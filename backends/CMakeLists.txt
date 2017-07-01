set (BACKEND_IR_DEFS "")
add_subdirectory(bmv2)
add_subdirectory(ebpf)
add_subdirectory(p4test)

# propagate the IR definitions
set (IR_DEF_FILES ${IR_DEF_FILES} ${BACKEND_IR_DEFS} PARENT_SCOPE)
