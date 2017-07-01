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

# Makefile for a backend that generates code for the Behavioral Model version 2 (BMv2)
# compiling for the simple_switch target.

# check for simple_switch
find_program (SIMPLE_SWITCH_CLI simple_switch_CLI
  PATHS ${CMAKE_INSTALL_PREFIX}/bin )
if (SIMPLE_SWITCH_CLI)
  find_program (SIMPLE_SWITCH simple_switch
    PATHS ${CMAKE_INSTALL_PREFIX}/bin )
  if (SIMPLE_SWITCH)
    set (HAVE_SIMPLE_SWITCH 1)
    find_python_module (scapy REQUIRED)
    find_python_module (ipaddr REQUIRED)
  endif(SIMPLE_SWITCH)
endif(SIMPLE_SWITCH_CLI)
find_package_handle_standard_args ("BMv2 programs"
  "Program 'simple_switch_CLI' (https://github.com/p4lang/behavioral-model.git) not found; will not run BMv2 tests."
  SIMPLE_SWITCH SIMPLE_SWITCH_CLI)

# sources for backend executable
set (BMV2_SRCS bmv2.cpp)

set (BMV2_BACKEND_SRCS
  analyzer.cpp
  action.cpp
  backend.cpp
  control.cpp
  deparser.cpp
  errorcode.cpp
  expression.cpp
  extern.cpp
  header.cpp
  helpers.cpp
  extractArchInfo.cpp
  JsonObjects.cpp
  lower.cpp
  midend.cpp
  metermap.cpp
  parser.cpp
  portableSwitch.cpp
  sharedActionSelectorCheck.cpp
  simpleSwitch.cpp
  synthesizeValidField.cpp
  )

set (BMV2_BACKEND_HDRS
  analyzer.h
  action.h
  backend.h
  control.h
  deparser.h
  errorcode.h
  expression.h
  extern.h
  header.h
  helpers.h
  extractArchInfo.h
  JsonObjects.h
  lower.h
  midend.h
  metermap.h
  options.h
  parser.h
  portableSwitch.h
  sharedActionSelectorCheck.h
  simpleSwitch.h
  synthesizeValidField.h
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/bmv2.def PARENT_SCOPE)

add_library(backendbmv2 ${BMV2_BACKEND_SRCS})
add_dependencies(backendbmv2 genIR)

add_executable(p4c-bm2-ss ${BMV2_SRCS})
target_link_libraries (p4c-bm2-ss backendbmv2 ${P4C_LIBRARIES} ${P4C_LIB_DEPS})

set(BMV2_CPPLINT_FILES ${BMV2_BACKEND_SRCS} ${BMV2_SRCS} ${BMV2_BACKEND_HDRS} PARENT_SCOPE)

# # Tests
# -include bmv2tests.mk

# CLEANFILES += bmv2tests.mk

# bmv2tests.mk: $(GENTESTS) $(srcdir)/%reldir%/Makefile.am \
# 	      $(srcdir)/testdata/p4_16_samples/*-bmv2.p4 \
# 	      $(srcdir)/testdata/p4_16_bmv_errors/*-bmv2.p4 \
# 	      $(srcdir)/testdata/p4_14_samples/*.p4 \
# 	      $(srcdir)/testdata/p4_14_samples/switch_*/switch.p4 \
# 	      $(srcdir)/testdata/p4_16_samples $(srcdir)/testdata/p4_14_samples
# 	@$(GENTESTS) $(srcdir) bmv2 $(srcdir)/backends/bmv2/run-bmv2-test.py $^ >$@

# gtest_LDADD += libbackendbmv2.la

# # This test defines two lpm keys for a table.
# # Even though the P4 spec allows it, it doesn't really make sense in a switch
# # so we allow it to fail on BMv2.
# XFAIL_TESTS += bmv2/testdata/p4_14_samples/issue60.p4.test
