ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-DTESTDATADIR=\"$(abs_srcdir)/testdata\"

TESTS = test.run test_packet_io.run

check_PROGRAMS = test test_packet_io

common_source = utils.h utils.cpp

test_SOURCES = test.cpp $(common_source)
test_packet_io_SOURCES = test_packet_io.cpp $(common_source)

LDADD = \
-lpiproto \
$(PROTOBUF_LIBS) $(GRPC_LIBS)
