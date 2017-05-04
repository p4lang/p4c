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

# General GTest unit tests. Add tests here if they don't have a logical home
# elsewhere in the codebase.
gtest_unittest_UNIFIED = \
	test/gtest/arch_test.cpp \
	test/gtest/helpers.cpp \
	test/gtest/midend_test.cpp \
	test/gtest/opeq_test.cpp

noinst_HEADERS += \
	test/gtest/helpers.h

cpplint_FILES += $(gtest_unittest_UNIFIED)
gtest_SOURCES += $(gtest_unittest_SOURCES)
