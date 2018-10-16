if COND_NANOMSG
MAYBE_LEARN_CLIENT = learn_client
endif

SUBDIRS = . $(MAYBE_LEARN_CLIENT)

noinst_PROGRAMS = l2_switch
l2_switch_SOURCES = l2_switch.cpp primitives.cpp
l2_switch_LDADD = $(top_builddir)/src/bm_runtime/libbmruntime.la \
$(top_builddir)/src/bm_sim/libbmsim.la \
$(top_builddir)/src/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/thrift_src/libruntimestubs.la \
$(top_builddir)/src/BMI/libbmi.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
-lboost_system $(THRIFT_LIB) -lboost_program_options -lboost_filesystem
