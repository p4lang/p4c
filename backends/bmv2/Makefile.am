# Makefile for a backend that generates code for the Behavioral Model version 2 (BMv2)
# compiling for the simple_switch target.
# To be included in the main P4C compiler makefile

bin_PROGRAMS += p4c-bm2-ss
p4c_bm2_ss_LDADD = -lgc libfrontend.a libp4ctoolkit.a

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
-include $(srcdir)/bmv2tests.am

CLEANFILES += $(srcdir)/bmv2tests.am

$(srcdir)/bmv2tests.am: $(srcdir)/testdata/v1_2_samples/*-bmv2.p4 $(srcdir)/testdata/v1_samples/*.p4
	@$(GENTESTS) $(srcdir) bmv2 $(srcdir)/backends/bmv2/run-bmv2-test.py $^ >$@

# These tests exhibit P4 v1.0 features still unimplemented
XFAIL_TESTS += \
    bmv2/testdata/v1_samples/09-IPv4OptionsUnparsed.p4.test \
    bmv2/testdata/v1_samples/TLV_parsing.p4.test \
    bmv2/testdata/v1_samples/truncate.p4.test
