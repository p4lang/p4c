ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . examples

AM_CPPFLAGS += \
-I$(top_builddir)/thrift_src/gen-cpp

libbmapps_la_SOURCES = \
learn.cpp \
packet_pipe.cpp \
nn.h

libbmapps_la_LIBADD = \
$(top_builddir)/thrift_src/libruntimestubs.la \
-lnanomsg $(THRIFT_LIB)
libbmapps_la_LDFLAGS = -pthread

noinst_LTLIBRARIES = libbmapps.la
