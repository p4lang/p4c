ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . examples

AM_CPPFLAGS += \
-I$(top_builddir)/thrift_src/gen-cpp

libbmapps_la_SOURCES = \
learn.cpp \
notifications.cpp \
packet_pipe.cpp \
nn.h

libbmapps_la_LIBADD = \
$(top_builddir)/thrift_src/libruntimestubs.la \
-lnanomsg $(THRIFT_LIB)

noinst_LTLIBRARIES = libbmapps.la
