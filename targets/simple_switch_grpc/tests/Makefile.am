ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-DTESTDATADIR=\"$(abs_srcdir)/testdata\"

TESTS = test.run

check_PROGRAMS = test

test_SOURCES = test.cpp

LDADD = \
-lpiproto \
$(PROTOBUF_LIBS) $(GRPC_LIBS)
