SUBDIRS = . learn_client

AM_CPPFLAGS += \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_runtime/include
bin_PROGRAMS = l2_switch
l2_switch_SOURCES = l2_switch.cpp l2_switch.h primitives.h simplelog.h
l2_switch_LDADD = $(srcdir)/../../modules/bm_runtime/libbmruntime.la $(srcdir)/../../modules/bm_sim/libbmsim.la $(srcdir)/../../modules/bf_lpm_trie/libbflpmtrie.la $(srcdir)/../../thrift_src/libruntimestubs.la $(srcdir)/../../modules/BMI/libbmi.la $(srcdir)/../../third_party/jsoncpp/libjson.la -lboost_system -lboost_thread -lthrift -lboost_program_options
l2_switch_LDFLAGS = -pthread
