# Copyright 2013-present Barefoot Networks, Inc.
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

################################################################################
# GTests
################################################################################

set (GTEST_UNITTEST_SOURCES
  gtest/arch_test.cpp
  gtest/bitvec_test.cpp
  gtest/call_graph_test.cpp
  gtest/complex_bitwise.cpp
  gtest/constant_expr_test.cpp
  gtest/cstring.cpp
  gtest/diagnostics.cpp
  gtest/dumpjson.cpp
  gtest/enumerator_test.cpp
  gtest/equiv_test.cpp
  gtest/exception_test.cpp
  gtest/expr_uses_test.cpp
  gtest/format_test.cpp
  gtest/helpers.cpp
  gtest/hvec_map.cpp
  gtest/indexed_vector.cpp
  gtest/json_test.cpp
  gtest/midend_test.cpp
  gtest/opeq_test.cpp
  gtest/ordered_map.cpp
  gtest/ordered_set.cpp
  gtest/parser_unroll.cpp
  gtest/path_test.cpp
  gtest/p4runtime.cpp
  gtest/source_file_test.cpp
  gtest/transforms.cpp
  gtest/stringify.cpp
)

# Combine the executable and the non-backend-specific unit tests into a single
# unity compilation group.
set (GTEST_BASE_SOURCES
  gtest/gtestp4c.cpp
  ${GTEST_UNITTEST_SOURCES}
)

# Merge the base sources with the tests added to GTEST_SOURCES by specific
# backends or by extensions. We can't assume that one extension's GTests can be
# unity with another's, so we can't handle unity compilation transparently
# for all GTests.
# XXX(seth): The current approach is an awkward fit for cmake anyway. We should
# restructure backend-specific GTests to live in their own libraries, and just
# link the libraries in.
set (GTESTP4C_SOURCES ${GTEST_SOURCES} ${GTEST_BASE_SOURCES})

# Build `gtestp4c`, which will contain all of our tests. Backends and extensions
# can add GTests to `GTEST_SOURCES` to include them in the test executable. They
# can also link in additional libraries, if needed, by adding them to
# `GTEST_LDADD`.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/test)
configure_file(gtest/env.h.in ${CMAKE_CURRENT_BINARY_DIR}/gtest/env.h)
add_executable (gtestp4c ${GTESTP4C_SOURCES})
target_link_libraries (gtestp4c ${GTEST_LDADD} ${P4C_LIBRARIES} gtest ${P4C_LIB_DEPS})
add_dependencies(gtestp4c gtest genIR frontend controlplane)

# The load_ir_from_json test needs this file. Easier to copy to build directory
# rather than pass absolute path in the case where build/ isn't located in the
# p4c root.
add_custom_target(copy_gtest_deps
  COMMAND ${CMAKE_COMMAND} -E copy ${P4C_SOURCE_DIR}/test/test_fromJSON.p4 ${P4C_BINARY_DIR}/test/)
add_dependencies(gtestp4c copy_gtest_deps)

# Tests
add_test (NAME gtestp4c COMMAND gtestp4c WORKING_DIRECTORY ${P4C_BINARY_DIR})
set_tests_properties (gtestp4c PROPERTIES LABELS "gtest")
