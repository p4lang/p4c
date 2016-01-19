AM_CPPFLAGS += \
-isystem $(top_srcdir)/third_party \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_apps/include \
-I$(srcdir)/.. \
-I$(srcdir)/
AM_CXXFLAGS = -pthread
LDADD = $(srcdir)/../libsimpleswitch.la \
$(top_srcdir)/third_party/gtest/libgtest.la \
$(top_srcdir)/modules/bm_apps/libbmapps.la \
-lboost_filesystem

# Define unit tests
common_source = main.cpp utils.cpp utils.h
TESTS = test_packet_redirect \
test_truncate

check_PROGRAMS = $(TESTS) test_all

# Sources for tests
test_packet_redirect_SOURCES = $(common_source) test_packet_redirect.cpp
test_truncate_SOURCES = $(common_source) test_truncate.cpp

test_all_SOURCES = $(common_source) \
test_packet_redirect.cpp \
test_truncate.cpp
