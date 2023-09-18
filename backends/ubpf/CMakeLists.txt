# Copyright 2019 Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# CMakefile for the uBPF P4-16 backend.

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set(P4C_UBPF_SOURCES
        p4c-ubpf.cpp
        ubpfBackend.cpp
        ubpfProgram.cpp
        ubpfParser.cpp
        ubpfDeparser.cpp
        ubpfControl.cpp
        ubpfType.cpp
        ubpfTable.cpp
        ubpfRegister.cpp
        ubpfModel.cpp
        target.cpp
        midend.cpp
        ../../backends/ebpf/ebpfProgram.cpp
        ../../backends/ebpf/ebpfTable.cpp
        ../../backends/ebpf/ebpfParser.cpp
        ../../backends/ebpf/ebpfDeparser.cpp
        ../../backends/ebpf/ebpfControl.cpp
        ../../backends/ebpf/ebpfOptions.cpp
        ../../backends/ebpf/target.cpp
        ../../backends/ebpf/codeGen.cpp
        ../../backends/ebpf/ebpfType.cpp
        ../../backends/ebpf/ebpfModel.cpp
        ../../backends/ebpf/midend.cpp
        ../../backends/ebpf/lower.cpp)

set(P4C_UBPF_HEADERS
        codeGen.h
        ubpfProgram.h
        ubpfType.h
        ubpfParser.h
        ubpfDeparser.h
        ubpfControl.h
        ubpfRegister.h
        ubpfModel.h
        target.h
        midend.h
        ubpfBackend.h)

set (P4C_UBPF_DIST_HEADERS p4include/ubpf_model.p4)


add_executable(p4c-ubpf ${P4C_UBPF_SOURCES})
target_link_libraries (p4c-ubpf ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-ubpf ir-generated frontend)

install (TARGETS p4c-ubpf
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
        DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})


add_custom_target(linkp4cubpf
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-ubpf ${P4C_BINARY_DIR}/p4c-ubpf
        COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${P4C_UBPF_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
        DEPENDS update_includes
        )

add_dependencies(p4c_driver linkp4cubpf)

set(UBPF_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/run-ubpf-test.py -t ubpf -c \"${P4C_BINARY_DIR}/p4c-ubpf\"")
set (UBPF_TEST_SUITES ${P4C_SOURCE_DIR}/testdata/p4_16_samples/*_ubpf.p4)
set (UBPF_XFAIL_TESTS)
p4c_add_tests("ubpf" ${UBPF_DRIVER} "${UBPF_TEST_SUITES}" "${UBPF_XFAIL_TESTS}")
p4c_add_test_with_args("ubpf" ${UBPF_DRIVER} FALSE "testdata/p4_16_samples/ubpf_hash_extern.p4" "testdata/p4_16_samples/ubpf_hash_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-hash-ubpf.c" "")
p4c_add_test_with_args("ubpf" ${UBPF_DRIVER} FALSE "testdata/p4_16_samples/ubpf_checksum_extern.p4" "testdata/p4_16_samples/ubpf_checksum_extern.p4" "--extern-file ${P4C_SOURCE_DIR}/testdata/extern_modules/extern-checksum-ubpf.c" "")
