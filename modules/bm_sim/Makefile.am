AM_CPPFLAGS += \
-I$(srcdir)/include/
AM_CPPFLAGS += $(DEBUG_CPPFLAGS)
# I used to have -Weffc++ for this module. I felt that most of it was excessively
# pedantic (like constructior initializers for non POD data), but it was
# useful. However in some versions of g++, it warns for non-virtual destructors
# in base classes even when the destructor is protected, so I removed it :/
AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(COVERAGE_FLAGS) # -Weffc++
AM_CFLAGS = $(PICKY_CFLAGS) $(COVERAGE_FLAGS)

noinst_LTLIBRARIES = libbmsim.la

common_source = \
src/actions.cpp \
src/checksums.cpp \
src/deparser.cpp \
src/fields.cpp \
src/P4Objects.cpp \
src/parser.cpp \
src/pipeline.cpp \
src/match_units.cpp \
src/match_tables.cpp \
src/xxhash.h \
src/conditionals.cpp \
src/event_logger.cpp \
src/headers.cpp \
src/packet.cpp \
src/phv.cpp \
src/switch.cpp \
src/xxhash.c \
src/learning.cpp \
src/simple_pre.cpp \
src/simple_pre_lag.cpp \
src/transport.cpp \
src/calculations.cpp \
src/meters.cpp \
src/counters.cpp \
src/dev_mgr.cpp \
src/options_parse.cpp \
src/ageing.cpp \
src/expressions.cpp \
src/pcap_file.cpp \
src/logger.cpp

common_include = \
include/bm_sim/actions.h \
include/bm_sim/ageing.h \
include/bm_sim/bignum.h \
include/bm_sim/bytecontainer.h \
include/bm_sim/checksums.h \
include/bm_sim/conditionals.h \
include/bm_sim/control_flow.h \
include/bm_sim/counters.h \
include/bm_sim/data.h \
include/bm_sim/deparser.h \
include/bm_sim/dev_mgr.h \
include/bm_sim/entries.h \
include/bm_sim/event_logger.h \
include/bm_sim/expressions.h \
include/bm_sim/fields.h \
include/bm_sim/field_lists.h \
include/bm_sim/handle_mgr.h \
include/bm_sim/headers.h \
include/bm_sim/logger.h \
include/bm_sim/lpm_trie.h \
include/bm_sim/meters.h \
include/bm_sim/named_p4object.h \
include/bm_sim/nn.h \
include/bm_sim/options_parse.h \
include/bm_sim/P4Objects.h \
include/bm_sim/packet_buffer.h \
include/bm_sim/packet.h \
include/bm_sim/parser.h \
include/bm_sim/pcap_file.h \
include/bm_sim/phv.h \
include/bm_sim/phv_forward.h \
include/bm_sim/pipeline.h \
include/bm_sim/queue.h \
include/bm_sim/runtime_interface.h \
include/bm_sim/stateful.h \
include/bm_sim/switch.h \
include/bm_sim/tables.h \
include/bm_sim/learning.h \
include/bm_sim/simple_pre.h \
include/bm_sim/simple_pre_lag.h \
include/bm_sim/transport.h

libbmsim_la_SOURCES = $(common_source) $(common_include)

clean-local:
	find $(srcdir) -type f -name '*.gcda' -o -name '*.gcov' -o -name "*.gcno" -delete
