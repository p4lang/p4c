AM_CPPFLAGS = -I$(srcdir)/../../modules/bm_sim/include -I$(srcdir)/../../modules/bf_lpm_trie/include -I$(srcdir)/../../third_party/jsoncpp/include -I$(srcdir)/../../modules/bm_runtime/include -I$(srcdir)/../../modules/BMI/include
bin_PROGRAMS = simple_switch
simple_switch_SOURCES = simple_switch.cpp primitives.h
simple_switch_LDADD = $(srcdir)/../../modules/bm_runtime/libbmruntime.la $(srcdir)/../../modules/bm_sim/libbmsim.la $(srcdir)/../../modules/bf_lpm_trie/libbflpmtrie.la $(srcdir)/../../thrift_src/libruntimestubs.la $(srcdir)/../../modules/BMI/libbmi.la $(srcdir)/../../third_party/jsoncpp/libjson.la -lboost_system -lboost_thread -lthrift -lboost_program_options
simple_switch_LDFLAGS = -pthread
