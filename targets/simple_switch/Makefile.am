SUBDIRS = . tests

simple_switch_thrift_files = \
gen-py/sswitch_runtime/__init__.py \
gen-cpp/simple_switch_constants.cpp \
gen-cpp/simple_switch_constants.h \
gen-cpp/SimpleSwitch.cpp \
gen-cpp/SimpleSwitch.h \
gen-cpp/simple_switch_types.cpp \
gen-cpp/simple_switch_types.h

BUILT_SOURCES = $(simple_switch_thrift_files)

AM_CPPFLAGS += \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_runtime/include \
-I$(srcdir)/thrift/src/ \
-I$(builddir)/gen-cpp

libsimpleswitch_la_LIBADD = \
$(top_builddir)/modules/bm_sim/libbmsim.la \
$(top_builddir)/modules/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/modules/BMI/libbmi.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
-lboost_system -lboost_thread -lthrift -lboost_program_options

libsimpleswitch_la_LDFLAGS = -pthread

noinst_LTLIBRARIES = libsimpleswitch.la

libsimpleswitch_la_SOURCES = \
simple_switch.cpp simple_switch.h primitives.cpp

bin_PROGRAMS = simple_switch

simple_switch_LDADD = \
$(top_builddir)/modules/bm_runtime/libbmruntime.la \
$(top_builddir)/thrift_src/libruntimestubs.la \
libsimpleswitch.la \
libsimpleswitch_thrift.la

simple_switch_SOURCES = main.cpp

lib_LTLIBRARIES = libsimpleswitch_thrift.la

libsimpleswitch_thrift_la_SOURCES = \
	$(BUILT_SOURCES)

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html

THRIFT_IDL = $(srcdir)/thrift/simple_switch.thrift

thrift_files.ts: $(THRIFT_IDL)
	@rm -f thrift_files.tmp
	@touch thrift_files.tmp
	$(THRIFT) --gen cpp -r $(THRIFT_IDL)
	$(THRIFT) --gen py -r $(THRIFT_IDL)
	@mv -f thrift_files.tmp $@

$(BUILT_SOURCES): thrift_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf thrift_files.lock thrift_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir thrift_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f thrift_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) thrift_files.ts; \
	  result=$$?; rm -rf thrift_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d thrift_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f thrift_files.ts; \
	  fi; \
	fi

pkginclude_HEADERS = \
gen-cpp/simple_switch_constants.h \
gen-cpp/SimpleSwitch.h \
gen-cpp/simple_switch_types.h


CLEANFILES = $(BUILT_SOURCES) \
thrift_files.ts
