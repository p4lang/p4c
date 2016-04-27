noinst_PROGRAMS = simple_router
simple_router_SOURCES = simple_router.cpp primitives.cpp
simple_router_LDADD = \
$(top_builddir)/src/bm_runtime/libbmruntime.la \
$(top_builddir)/src/bm_sim/libbmsim.la \
$(top_builddir)/src/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/thrift_src/libruntimestubs.la \
$(top_builddir)/src/BMI/libbmi.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
-lboost_system -lboost_thread $(THRIFT_LIB) -lboost_program_options
simple_router_LDFLAGS = -pthread
