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

# Unit tests
TESTSUITE = $(srcdir)/tests/testsuite

TESTSUITE_AT += \
	tests/testsuite.at \
	tests/frontend.at

if TESTFLAGS
EXTRA_FLAGS=$T
endif

# allow -jN, but ignore -j, as -j uses all cores in testsuite
check-local: atconfig $(TESTSUITE) $(TESTSUITE_AT)
	'$(TESTSUITE)' -C tests $(TESTSUITEFLAGS) \
		$(filter-out -j,$(filter -j%,$(MAKEFLAGS))) \
		$(EXTRA_FLAGS); \
	"$$@" || (test X'$(RECHECK)' = Xyes && "$$@" --recheck)

# helper function rules
recheck:
	set $(SHELL) '$(TESTSUITE)' -C tests --recheck;

check-help:
	@echo "make check-local            : run all the tests"
	@echo "make check-local T=[OPTION] : run with extra options"
	@echo "  OPTIONS:"
	@echo "  -l, --list              describe all the tests, or the selected TESTS"
	@echo "  -k, --keywords=KEYWORDS "
	@echo "                          select the tests matching all the comma-separated KEYWORDS"
	@echo "                          multiple '-k' accumulates; prefixed '!' negates a KEYWORD"
	@echo "  --recheck               select all tests that failed or passed unexpectedly last time"
	@echo "  -v, --verbose           force more detailed output"
	@echo "  -e, --errexit           abort as soon as a test fails"
	@echo "  \"1 4-5 12-\"           specify range of tests to run"
	@echo ""
	@echo "SHORTCUTS:"
	@echo "make check-bmv2             : run all bmv2 tests"
	@echo "make check-frontend         : run all frontend tests"
	@echo "make check-ebpf             : run all ebpf tests"

check-bmv2:
	+$(MAKE) check-local TESTSUITEFLAGS='-k bmv2 $(TESTSUITEFLAGS) $(EXTRA_FLAGS)'

check-frontend:
	+$(MAKE) check-local TESTSUITEFLAGS='-k frontend $(TESTSUITEFLAGS) $(EXTRA_FLAGS)'

check-ebpf:
	+$(MAKE) check-local TESTSUITEFLAGS='-k ebpf $(TESTSUITEFLAGS) $(EXTRA_FLAGS)'

clean-local:
	@test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean

TESTSUITE_PATCH = $(srcdir)/tests/testsuite.patch
AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT) $(XFAIL_FILES) $(TESTSUITE_PATCH)
	$(AUTOTEST) -I $(srcdir) $@.at -o testsuite.tmp
	patch -p0 testsuite.tmp $(TESTSUITE_PATCH)
	mv testsuite.tmp $@

%.text: #do nothing

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	$(AM_V_GEN):;{ \
		echo '# Signature of the current package.' && \
		echo 'm4_define([AT_PACKAGE_NAME], [$(PACKAGE_NAME)])' && \
		echo 'm4_define([AT_PACKAGE_TARNAME], [$(PACKAGE_TARNAME)])' && \
		echo 'm4_define([AT_PACKAGE_VERSION], [$(PACKAGE_VERSION)])' && \
		echo 'm4_define([AT_PACKAGE_STRING], [$(PACKAGE_STRING)])' && \
		echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])'; \
	} >'$(srcdir)/package.m4'

noinst_PROGRAMS += tests/p4ctest
tests_p4ctest_SOURCES += \
	tests/p4ctest.cpp \
	tests/p4ctest.h \
	tests/test-call-graph.cpp \
	tests/test-default.cpp \
	tests/test-enumerator.cpp \
	tests/test-exception.cpp \
	tests/test-format.cpp \
	tests/test-json.cpp \
	tests/test-opeq.cpp \
	tests/test-path.cpp \
	tests/test-source-file.cpp \
	tests/test-transform.cpp

tests_p4ctest_LDADD = libfrontend.la libp4ctoolkit.a
tests_p4ctest_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)/tests
