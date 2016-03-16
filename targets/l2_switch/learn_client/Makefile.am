AM_CPPFLAGS = \
-I$(top_builddir)/thrift_src/gen-cpp \
-I$(top_srcdir)/modules/bm_apps/include

if P4THRIFT
  AM_CPPFLAGS += -DP4THRIFT 
endif

noinst_PROGRAMS = learn_client
learn_client_SOURCES = learn_client.cpp
learn_client_LDADD = $(top_builddir)/modules/bm_apps/libbmapps.la
learn_client_LDFLAGS = -pthread
