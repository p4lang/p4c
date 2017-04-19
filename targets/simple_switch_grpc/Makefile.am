ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . tests

AM_CPPFLAGS += \
-I$(srcdir)/../../include/ \
-I$(srcdir)/../simple_switch

bin_PROGRAMS = simple_switch_grpc

simple_switch_grpc_SOURCES = main.cpp

simple_switch_grpc_LDADD = \
$(builddir)/../simple_switch/libsimpleswitch.la \
$(builddir)/../../PI/libbmpi.la \
-lpifeproto -lpigrpcserver -lpi

# We follow this tutorial to link with grpc++_reflection:
# https://github.com/grpc/grpc/blob/master/doc/server_reflection_tutorial.md
simple_switch_grpc_LDFLAGS = \
-Wl,--no-as-needed,-lgrpc++_reflection,--as-needed
