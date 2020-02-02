ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

# Ensures that gtest has been built
SUBDIRS = ../../../third_party

AM_CPPFLAGS += \
-isystem $(top_srcdir)/../../third_party/gtest/include \
-I$(top_srcdir) \
-I$(top_srcdir)/../../include \
-isystem$(top_srcdir)/../../third_party/spdlog \
-I$(top_builddir)/cpp_out -I$(top_builddir)/grpc_out \
-DTESTDATADIR=\"$(abs_srcdir)/testdata\"

if WITH_SYSREPO
AM_CPPFLAGS += -DWITH_SYSREPO
endif

if WITH_THRIFT
AM_CPPFLAGS += -DWITH_THRIFT
endif

# I am putting all the tests in the same binary (test_gtest) to use a single
# switch instance (this way there is no need to worry about allocating
# non-conflicting port-numbers and device ids). The drawback is that the tests
# cannot run in parallel.

TESTS = example.run test_gtest

check_PROGRAMS = example test_gtest

common_source = utils.h utils.cpp base_test.h base_test.cpp

example_SOURCES = example.cpp utils.h utils.cpp
test_gtest_SOURCES = \
$(common_source) main.cpp \
test_basic.cpp test_grpc_dp.cpp test_packet_io.cpp \
test_counter.cpp test_meter.cpp \
test_ternary.cpp \
test_pre.cpp \
test_digest.cpp \
test_idle_timeout.cpp \
test_action_profile.cpp \
test_optional.cpp
if WITH_SYSREPO
test_gtest_SOURCES += test_gnmi.cpp
endif

LDADD = \
$(top_builddir)/libsimple_switch_grpc.la \
$(top_builddir)/../simple_switch/libsimpleswitch.la \
-lpiprotogrpc -lpiprotobuf \
$(PROTOBUF_LIBS) $(GRPC_LIBS) \
$(top_builddir)/../../third_party/gtest/libgtest.la

EXTRA_DIST = \
example.run.in \
simple_router.json simple_router.proto.txt \
packet_io.p4 packet_io.json packet_io.proto.txt \
loopback.p4 loopback.json loopback.proto.txt \
counter.p4 counter.json counter.proto.txt \
meter.p4 meter.json meter.proto.txt \
ternary.p4 ternary.json ternary.proto.txt \
multicast.p4 multicast.json multicast.proto.txt \
clone.p4 clone.json clone.proto.txt \
digest.p4 digest.json digest.proto.txt \
action_profile.p4 action_profile.json action_profile.proto.txt \
optional.p4 optional.json optional.proto.txt
