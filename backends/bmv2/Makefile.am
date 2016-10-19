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
# To be included in the main P4C compiler makefile

bin_PROGRAMS += p4c-bm2-ss
p4c_bm2_ss_LDADD = libfrontend.a libp4ctoolkit.a

p4c_bm2_ss_SOURCES = \
	backends/bmv2/bmv2.cpp \
	backends/bmv2/analyzer.h \
	backends/bmv2/analyzer.cpp \
	backends/bmv2/jsonconverter.h \
	backends/bmv2/jsonconverter.cpp \
	backends/bmv2/inlining.h \
	backends/bmv2/inlining.cpp \
	backends/bmv2/midend.h \
	backends/bmv2/midend.cpp \
	backends/bmv2/lower.h \
	backends/bmv2/lower.cpp

cpplint_FILES += $(p4c_bm2_ss_SOURCES)

ir_DEF_FILES += $(srcdir)/backends/bmv2/bmv2.def

# Tests
-include bmv2tests.mk

CLEANFILES += bmv2tests.mk

bmv2tests.mk: $(GENTESTS) $(srcdir)/%reldir%/Makefile.am \
	      $(srcdir)/testdata/p4_16_samples/*-bmv2.p4 \
	      $(srcdir)/testdata/p4_16_bmv_errors/*-bmv2.p4 \
	      $(srcdir)/testdata/p4_14_samples/*.p4 \
	      $(srcdir)/testdata/p4_14_samples/switch_*/switch.p4 \
	      $(srcdir)/testdata/p4_16_samples $(srcdir)/testdata/p4_14_samples
	@$(GENTESTS) $(srcdir) bmv2 $(srcdir)/backends/bmv2/run-bmv2-test.py $^ >$@

# First 3 exhibit P4_14 features still unimplemented
# Last two are back-end bugs
XFAIL_TESTS += \
    bmv2/testdata/p4_14_samples/09-IPv4OptionsUnparsed.p4.test \
    bmv2/testdata/p4_14_samples/TLV_parsing.p4.test \
    bmv2/testdata/p4_14_samples/truncate.p4.test \
    bmv2/testdata/p4_16_samples/drop-bmv2.p4.test
IFAIL_TESTS += \
    bmv2/testdata/p4_16_samples/clone-bmv2.p4.test
