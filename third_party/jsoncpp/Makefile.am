AM_CPPFLAGS = -I$(srcdir)/include
# AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CFLAGS = $(PICKY_CFLAGS)

noinst_LTLIBRARIES = libjson.la

common_source = \
src/jsoncpp.cpp

common_include = \
include/jsoncpp/json.h

libjson_la_SOURCES = $(common_source) $(common_include)
