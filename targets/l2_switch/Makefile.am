SUBDIRS = . learn_client

noinst_PROGRAMS = l2_switch
l2_switch_SOURCES = l2_switch.cpp primitives.cpp
l2_switch_LDADD = $(top_builddir)/src/bm_runtime/libbmruntime.la \
$(top_builddir)/src/bm_sim/libbmsim.la \
$(top_builddir)/src/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/thrift_src/libruntimestubs.la \
$(top_builddir)/src/BMI/libbmi.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
-lboost_system -lboost_thread $(THRIFT_LIB) -lboost_program_options
l2_switch_LDFLAGS = -pthread
