AM_CPPFLAGS += \
-isystem $(top_srcdir)/third_party \
-DTESTDATADIR=\"$(abs_srcdir)/testdata\"
AM_CXXFLAGS = -pthread
LDADD = \
$(top_builddir)/third_party/gtest/libgtest.la \
$(top_builddir)/src/bm_apps/libbmapps.la \
$(top_builddir)/src/bm_sim/libbmsim.la \
$(top_builddir)/src/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
-lboost_system -lboost_thread -lboost_filesystem -lboost_program_options

# TODO(antonin): should the traffic.bin files be generated at compile time by
# running the Python generator (instead of being checked in)?

common_source = ../bmi_stubs.c stress_utils.h stress_utils.cpp

TESTS = \
test_parser_deparser_1 \
test_exact_match_1 \
test_LPM_match_1 \
test_ternary_match_1

check_PROGRAMS = $(TESTS)

test_parser_deparser_1_SOURCES = $(common_source) test_parser_deparser_1.cpp
test_exact_match_1_SOURCES = $(common_source) test_exact_match_1.cpp
test_LPM_match_1_SOURCES = $(common_source) test_LPM_match_1.cpp
test_ternary_match_1_SOURCES = $(common_source) test_ternary_match_1.cpp

EXTRA_DIST = \
testdata/parser_deparser_1.p4 \
testdata/parser_deparser_1.json \
testdata/exact_match_1.p4 \
testdata/exact_match_1.json \
testdata/LPM_match_1.p4 \
testdata/LPM_match_1.json \
testdata/ternary_match_1.p4 \
testdata/ternary_match_1.json \
testdata/udp_tcp_traffic.bin
