AM_CPPFLAGS += \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/thrift_src/gen-cpp
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CFLAGS = $(PICKY_CFLAGS)
noinst_LTLIBRARIES = libbmruntime.la

common_source = \
src/server.cpp

common_include = \
include/bm_runtime/bm_runtime.h

libbmruntime_la_SOURCES = $(common_source) $(common_include)
