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

# Flags needed for GTest tests.
set (GTEST_ROOT ${P4C_SOURCE_DIR}/test/frameworks/gtest/googletest)

include_directories(${GTEST_ROOT} ${GTEST_ROOT}/include)

# Configure GTest to disable support for SEH (which is irrelevant for p4c since
# we don't currently support Windows) and pthreads (which prevents GTest from
# using its internal TLS implementation, which interacts badly with libgc).
add_definitions(-DGTEST_HAS_SEH=0)
add_definitions(-DGTEST_HAS_PTHREAD=0)

# Build the GTest library itself.
add_library(gtest ${GTEST_ROOT}/src/gtest-all.cc)

set (GTEST_UNITTEST_SOURCES
  gtest/arch_test.cpp
  gtest/bmv2_isvalid.cpp
  gtest/call_graph_test.cpp
  gtest/dumpjson.cpp
  gtest/enumerator_test.cpp
  gtest/exception_test.cpp
  gtest/format_test.cpp
  gtest/helpers.cpp
  gtest/json_test.cpp
  gtest/midend_test.cpp
  gtest/opeq_test.cpp
  gtest/path_test.cpp
  gtest/source_file_test.cpp
  gtest/transforms.cpp
  )
set (GTEST_UNITTEST_HEADERS
  gtest/helpers.h
  )

set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_UNITTEST_SOURCES} ${GTEST_UNITTEST_HEADERS})

set (GTESTP4C_SOURCES
  gtest/gtestp4c.cpp
  ${GTEST_SOURCES}
  )

# Build `gtestp4c`, which will contain all of our tests. Makefiles can add tests
# to $(GTEST_SOURCES) to include them in the test executable.
# Makefiles can add libraries to $(GTEST_LDADD) to include them in the test
# executable.
add_executable (gtestp4c ${GTESTP4C_SOURCES})
target_link_libraries (gtestp4c ${GTEST_LDADD} bmv2backend ${P4C_LIBRARIES} gtest ${P4C_LIB_DEPS})

# Tests
add_test (NAME gtestp4c COMMAND gtestp4c WORKING_DIRECTORY ${P4C_BINARY_DIR})
set_tests_properties (gtestp4c PROPERTIES LABELS "gtest")

# add to cpplint
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${GTESTP4C_SOURCES}")
