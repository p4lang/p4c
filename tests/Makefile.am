AM_CPPFLAGS += \
-isystem $(top_srcdir)/third_party \
-I$(top_srcdir)/modules/bm_sim/include \
-I$(top_srcdir)/modules/bm_sim/src # need to include pcap_file.cc, because it contains a private class implementation needed for tests
AM_CXXFLAGS = -pthread
LDADD = \
$(top_srcdir)/third_party/gtest/libgtest.la \
$(top_srcdir)/modules/bm_sim/libbmsim.la \
$(top_srcdir)/modules/bf_lpm_trie/libbflpmtrie.la \
$(top_srcdir)/third_party/jsoncpp/libjson.la \
-lboost_system -lboost_thread

# Define unit tests
common_source = main.cpp
TESTS = test_actions \
test_checksums \
test_conditionals \
test_data \
test_handle_mgr \
test_p4objects \
test_parser \
test_phv \
test_queue \
test_tables \
test_learning \
test_pre \
test_calculations \
test_header_stacks \
test_meters \
test_ageing \
test_counters \
test_pcap \
test_fields
check_PROGRAMS = $(TESTS) test_all

# Sources for tests
test_actions_SOURCES       = $(common_source) test_actions.cpp
test_checksums_SOURCES     = $(common_source) test_checksums.cpp
test_conditionals_SOURCES  = $(common_source) test_conditionals.cpp
test_data_SOURCES          = $(common_source) test_data.cpp
test_handle_mgr_SOURCES    = $(common_source) test_handle_mgr.cpp
test_p4objects_SOURCES     = $(common_source) test_p4objects.cpp
test_parser_SOURCES        = $(common_source) test_parser.cpp
test_phv_SOURCES           = $(common_source) test_phv.cpp
test_queue_SOURCES         = $(common_source) test_queue.cpp
test_tables_SOURCES        = $(common_source) test_tables.cpp
test_learning_SOURCES      = $(common_source) test_learning.cpp
test_pre_SOURCES           = $(common_source) test_pre.cpp
test_calculations_SOURCES  = $(common_source) test_calculations.cpp
test_header_stacks_SOURCES = $(common_source) test_header_stacks.cpp
test_meters_SOURCES        = $(common_source) test_meters.cpp
test_ageing_SOURCES        = $(common_source) test_ageing.cpp
test_counters_SOURCES      = $(common_source) test_counters.cpp
test_fields_SOURCES   	   = $(common_source) test_fields.cpp
test_pcap_SOURCES          = $(common_source) test_pcap.cpp
test_all_SOURCES = $(common_source) \
test_actions.cpp \
test_checksums.cpp \
test_conditionals.cpp \
test_data.cpp \
test_handle_mgr.cpp \
test_p4objects.cpp \
test_parser.cpp \
test_phv.cpp \
test_queue.cpp \
test_tables.cpp \
test_learning.cpp \
test_pre.cpp \
test_calculations.cpp \
test_header_stacks.cpp \
test_meters.cpp \
test_ageing.cpp \
test_counters.cpp \
test_pcap.cpp \
test_fields.cpp
