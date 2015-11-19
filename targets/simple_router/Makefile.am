AM_CPPFLAGS += \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_runtime/include
bin_PROGRAMS = simple_router
simple_router_SOURCES = simple_router.cpp primitives.h
simple_router_LDADD = $(srcdir)/../../modules/bm_runtime/libbmruntime.la $(srcdir)/../../modules/bm_sim/libbmsim.la $(srcdir)/../../modules/bf_lpm_trie/libbflpmtrie.la $(srcdir)/../../thrift_src/libruntimestubs.la $(srcdir)/../../modules/BMI/libbmi.la $(srcdir)/../../third_party/jsoncpp/libjson.la -lboost_system -lboost_thread -lthrift -lboost_program_options
simple_router_LDFLAGS = -pthread
