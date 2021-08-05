SUBDIRS = .

AM_CPPFLAGS += \
-I$(top_srcdir)/targets/psa_switch/externs \
-isystem $(top_srcdir)/third_party/gtest/include \
-I$(srcdir)/.. \
-I$(srcdir)/ \
-DTESTDATADIR=\"$(srcdir)/testdata\"
LDADD = $(builddir)/../libpsaswitch.la \
$(top_builddir)/third_party/gtest/libgtest.la \
$(top_builddir)/src/bm_apps/libbmapps.la \
-lboost_filesystem

# Define unit tests
common_source = main.cpp
TESTS = test_internet_checksum

check_PROGRAMS = $(TESTS) test_all

# Sources for tests
test_internet_checksum_SOURCES = $(common_source) test_internet_checksum.cpp

test_all_SOURCES = $(common_source) \
test_internet_checksum.cpp
