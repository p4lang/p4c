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
GTEST_ROOT = $(top_srcdir)/test/frameworks/gtest/googletest
GTEST_CPPFLAGS = -I$(GTEST_ROOT) -I$(GTEST_ROOT)/include

# Make the GTest headers globally available for code which needs to include
# `gtest_prod.h`.
AM_CPPFLAGS += $(GTEST_CPPFLAGS)

# Build the GTest library itself.
check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = test/frameworks/gtest/googletest/src/gtest-all.cc

# Build `gtestp4c`, which will contain all of our tests. Makefiles can add tests
# to $(gtest_SOURCES) to include them in the test executable.
gtestp4c_SOURCES = \
	test/gtest/gtestp4c.cpp \
	$(gtest_SOURCES)

# Makefiles can add libraries to $(gtest_LDADD) to include them in the test
# executable.
gtestp4c_LDADD = libgtest.la libfrontend.la libp4ctoolkit.a $(gtest_LDADD)

check_PROGRAMS += gtestp4c
cpplint_FILES += test/gtest/gtestp4c.cpp


################################################################################
# Compiler tests
################################################################################

TESTS += $(check_PROGRAMS)
