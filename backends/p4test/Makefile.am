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

# Makefile for a fake backend that is used for testing the P4-16 front-end.
# To be included in the main P4C compiler makefile

noinst_PROGRAMS += p4test
p4test_LDADD = libfrontend.la libp4ctoolkit.a

p4test_UNIFIED = \
    backends/p4test/p4test.cpp \
    backends/p4test/midend.cpp

noinst_HEADERS += backends/p4test/midend.h

cpplint_FILES += $(p4test_UNIFIED) $(p4test_NONUNIFIED)

# Unit tests
tests_p4ctest_SOURCES += \
	backends/p4test/tests/test-backend-p4test.cpp

TESTSUITE_AT += \
	backends/p4test/tests/testsuite-p4test.at \
	backends/p4test/tests/p4test.at
