AM_CPPFLAGS = -isystem $(srcdir)/../../third_party/jsoncpp/include -I$(srcdir)/../bm_sim/include -I$(srcdir)/../bf_lpm_trie/include -I$(srcdir)/../BMI/include -I$(srcdir)/../../thrift_src/gen-cpp
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CFLAGS = $(PICKY_CFLAGS)
noinst_LTLIBRARIES = libbmruntime.la

common_source = \
src/server.cpp

common_include = \
include/bm_runtime/bm_runtime.h

libbmruntime_la_SOURCES = $(common_source) $(common_include)
