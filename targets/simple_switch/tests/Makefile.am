if COND_THRIFT
MAYBE_CLI_TESTS = CLI_tests
endif

SUBDIRS = . $(MAYBE_CLI_TESTS)

AM_CPPFLAGS += \
-isystem $(top_srcdir)/third_party/gtest/include \
-I$(srcdir)/.. \
-I$(srcdir)/ \
-DTESTDATADIR=\"$(srcdir)/testdata\"
LDADD = $(builddir)/../libsimpleswitch.la \
$(top_builddir)/third_party/gtest/libgtest.la \
$(top_builddir)/src/bm_apps/libbmapps.la \
-lboost_filesystem

# Define unit tests
common_source = main.cpp utils.cpp utils.h
TESTS = test_packet_redirect \
test_truncate \
test_swap \
test_queueing \
test_recirc \
test_parser_error

check_PROGRAMS = $(TESTS) test_all

# Sources for tests
test_packet_redirect_SOURCES = $(common_source) test_packet_redirect.cpp
test_truncate_SOURCES = $(common_source) test_truncate.cpp
test_swap_SOURCES = $(common_source) test_swap.cpp
test_queueing_SOURCES = $(common_source) test_queueing.cpp
test_recirc_SOURCES = $(common_source) test_recirc.cpp
test_parser_error_SOURCES = $(common_source) test_parser_error.cpp

test_all_SOURCES = $(common_source) \
test_packet_redirect.cpp \
test_truncate.cpp \
test_swap.cpp \
test_queueing.cpp \
test_recirc.cpp \
test_parser_error.cpp

EXTRA_DIST = \
testdata/packet_redirect.json \
testdata/truncate.json \
testdata/swap_1.json \
testdata/swap_2.json \
testdata/queueing.json \
testdata/recirc.json \
testdata/parser_error.p4 \
testdata/parser_error.json
