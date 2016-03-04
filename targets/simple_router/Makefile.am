AM_CPPFLAGS += \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_runtime/include
noinst_PROGRAMS = simple_router
simple_router_SOURCES = simple_router.cpp primitives.cpp
simple_router_LDADD = $(top_builddir)/modules/bm_runtime/libbmruntime.la $(top_builddir)/modules/bm_sim/libbmsim.la $(top_builddir)/modules/bf_lpm_trie/libbflpmtrie.la $(top_builddir)/thrift_src/libruntimestubs.la $(top_builddir)/modules/BMI/libbmi.la $(top_builddir)/third_party/jsoncpp/libjson.la -lboost_system -lboost_thread -lthrift -lboost_program_options
simple_router_LDFLAGS = -pthread
