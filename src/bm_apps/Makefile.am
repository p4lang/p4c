ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . examples

libbmapps_la_SOURCES = \
notifications.cpp \
packet_pipe.cpp \
nn.h

libbmapps_la_LIBADD = \
-lnanomsg

if COND_THRIFT
AM_CPPFLAGS += \
-I$(top_builddir)/thrift_src/gen-cpp

libbmapps_la_SOURCES += learn.cpp

libbmapps_la_LIBADD += \
$(top_builddir)/thrift_src/libruntimestubs.la \
$(THRIFT_LIB)
endif

noinst_LTLIBRARIES = libbmapps.la
