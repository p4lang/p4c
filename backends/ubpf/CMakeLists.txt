# CMakefile for the uBPF P4-16 backend.

set(P4C_UBPF_SOURCES
        p4c-ubpf.cpp
        ubpfBackend.cpp
        ubpfProgram.cpp
        target.cpp
        ../../backends/ebpf/ebpfProgram.cpp
        ../../backends/ebpf/ebpfTable.cpp
        ../../backends/ebpf/ebpfParser.cpp
        ../../backends/ebpf/ebpfControl.cpp
        ../../backends/ebpf/target.cpp
        ../../backends/ebpf/codeGen.cpp
        ../../backends/ebpf/ebpfType.cpp
        ../../backends/ebpf/ebpfModel.cpp
        ../../backends/ebpf/midend.cpp
        ../../backends/ebpf/lower.cpp)

set(P4C_UBPF_HEADERS
        ubpfProgram.h
        target.h
        ubpfBackend.h)

set (P4C_UBPF_DIST_HEADERS p4include/filter_model.p4)

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "$(P4C_UBPF_SOURCES)")

#build_unified(P4C_UBPF_SOURCES ALL)
add_executable(p4c-ubpf ${P4C_UBPF_SOURCES})
target_link_libraries (p4c-ubpf ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
#add_dependencies(p4c-ubpf genIR frontend)

install (TARGETS p4c-ubpf
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
        DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})


add_custom_target(linkp4cubpf
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-ubpf ${P4C_BINARY_DIR}/p4c-ubpf
        COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${P4C_UBPF_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
        )

add_dependencies(p4c_driver linkp4cubpf)