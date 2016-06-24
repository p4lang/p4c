ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-I$(top_builddir)/thrift_src/gen-cpp

noinst_LTLIBRARIES = libbmruntime.la

libbmruntime_la_SOURCES = \
server.cpp \
Standard_server.cpp \
SimplePre_server.cpp \
SimplePreLAG_server.cpp
