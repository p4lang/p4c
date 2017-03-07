ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-I$(srcdir)/../../include/ \
-I$(srcdir)/../simple_switch

bin_PROGRAMS = simple_switch_grpc

simple_switch_grpc_SOURCES = main.cpp

simple_switch_grpc_LDADD = \
$(builddir)/../simple_switch/libsimpleswitch.la \
$(builddir)/../../PI/libbmpi.la \
-lpifeproto -lpigrpcserver -lpi
