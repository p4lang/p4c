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
# Unit tests
################################################################################

check_PROGRAMS += exception_test format_test source_file_test path_test \
		  enumerator_test default_test unittest_transform1 json_test \
		  call_graph_test dumpjson

default_test_SOURCES = test/unittests/default_test.cpp
default_test_LDADD = libp4ctoolkit.a
exception_test_SOURCES = test/unittests/exception_test.cpp
exception_test_LDADD = libp4ctoolkit.a
enumerator_test_SOURCES = test/unittests/enumerator_test.cpp
enumerator_test_LDADD = libp4ctoolkit.a
source_file_test_SOURCES = test/unittests/source_file_test.cpp
source_file_test_LDADD = libp4ctoolkit.a
format_test_SOURCES = test/unittests/format_test.cpp
format_test_LDADD = libp4ctoolkit.a
path_test_SOURCES = test/unittests/path_test.cpp
path_test_LDADD = libp4ctoolkit.a
json_test_SOURCES = test/unittests/json_test.cpp
json_test_LDADD = libp4ctoolkit.a
call_graph_test_SOURCES = test/unittests/call_graph_test.cpp
call_graph_test_LDADD = libp4ctoolkit.a libfrontend.la
unittest_transform1_SOURCES = test/unittests/transform1.cpp
unittest_transform1_LDADD = libfrontend.la libp4ctoolkit.a
dumpjson_SOURCES = test/unittests/dumpjson.cpp
dumpjson_LDADD = libfrontend.la libp4ctoolkit.a


################################################################################
# GTests
################################################################################

# Flags needed for GTest tests.
GTEST_ROOT = $(top_srcdir)/test/frameworks/gtest/googletest
GTEST_CPPFLAGS = -I$(GTEST_ROOT) -I$(GTEST_ROOT)/include $(AM_CPPFLAGS)

# Build the GTest library itself.
check_LTLIBRARIES = libgtest.la
libgtest_la_CPPFLAGS = $(GTEST_CPPFLAGS)
libgtest_la_SOURCES = test/frameworks/gtest/googletest/src/gtest-all.cc

# Build `gtestp4c`, which will contain all of our tests. Makefiles can add tests
# to $(gtest_SOURCES) to include them in the test executable.
gtestp4c_SOURCES = \
	test/gtest/gtestp4c.cpp \
	$(gtest_SOURCES)

check_PROGRAMS += gtestp4c
gtestp4c_LDADD = libgtest.la libfrontend.la libp4ctoolkit.a
gtestp4c_CPPFLAGS = $(GTEST_CPPFLAGS)
cpplint_FILES += test/gtest/gtestp4c.cpp
TESTS += gtestp4c


################################################################################
# Compiler tests
################################################################################

TESTS += $(check_PROGRAMS)
