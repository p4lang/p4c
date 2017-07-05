ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-isystem $(top_srcdir)/../../third_party/gtest/include \
-I$(top_srcdir) \
-I$(top_srcdir)/../../include \
-I$(top_builddir)/cpp_out -I$(top_builddir)/grpc_out \
-DTESTDATADIR=\"$(abs_srcdir)/testdata\"

TESTS = test.run test_packet_io.run test_grpc_dp

check_PROGRAMS = test test_packet_io test_grpc_dp

common_source = utils.h utils.cpp base_test.h base_test.cpp

test_SOURCES = test.cpp $(common_source)
test_packet_io_SOURCES = test_packet_io.cpp $(common_source)
test_grpc_dp_SOURCES = test_grpc_dp.cpp $(common_source) main.cpp

LDADD = \
$(top_builddir)/libsimple_switch_grpc.la \
$(top_builddir)/../simple_switch/libsimpleswitch.la \
-lpiproto \
$(PROTOBUF_LIBS) $(GRPC_LIBS) \
$(top_builddir)/../../third_party/gtest/libgtest.la

EXTRA_DIST = \
simple_router.json simple_router.proto.txt \
packet_io.p4 packet_io.json packet_io.proto.txt \
loopback.p4 loopback.json loopback.proto.txt
