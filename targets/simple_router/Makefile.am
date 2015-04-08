AM_CPPFLAGS = -I$(srcdir)/../../modules/bm_sim/include -I$(srcdir)/../../modules/bf_lpm_trie/include -I$(srcdir)/../../third_party/jsoncpp/include -I$(srcdir)/../../modules/bm_runtime/include -I$(srcdir)/../../modules/BMI/include
bin_PROGRAMS = simple_router
simple_router_SOURCES = simple_router.cpp main.cpp primitives.h
simple_router_LDADD = $(srcdir)/../../modules/bm_runtime/libbmruntime.a $(srcdir)/../../modules/bm_sim/libbmsim.a $(srcdir)/../../modules/bf_lpm_trie/libbflpmtrie.a $(srcdir)/../../thrift_src/libruntimestubs.a $(srcdir)/../../modules/BMI/libbmi.a $(srcdir)/../../third_party/jsoncpp/libjson.a -lboost_system -lboost_thread -lthrift -lboost_program_options
simple_router_LDFLAGS = -pthread
