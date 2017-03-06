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
TESTSUITE = tests/testsuite

TESTSUITE_AT += \
	tests/testsuite.at \
	tests/frontend.at

check-local: atconfig $(TESTSUITE) $(TESTSUITE_AT)
	set $(SHELL) '$(TESTSUITE)' -C tests $(TESTSUITEFLAGS); "$$@"

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

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
