AM_CPPFLAGS = \
-I$(top_srcdir)/thrift_src/gen-cpp \
-I$(top_srcdir)/modules/bm_apps/include
bin_PROGRAMS = learn_client
learn_client_SOURCES = learn_client.cpp
learn_client_LDADD = $(top_srcdir)/modules/bm_apps/libbmapps.la
learn_client_LDFLAGS = -pthread
