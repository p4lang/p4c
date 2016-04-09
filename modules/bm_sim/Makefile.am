AM_CPPFLAGS += \
-I$(srcdir)/include/
# I used to have -Weffc++ for this module. I felt that most of it was excessively
# pedantic (like constructior initializers for non POD data), but it was
# useful. However in some versions of g++, it warns for non-virtual destructors
# in base classes even when the destructor is protected, so I removed it :/
AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(COVERAGE_FLAGS) # -Weffc++
AM_CFLAGS = $(PICKY_CFLAGS) $(COVERAGE_FLAGS)

noinst_LTLIBRARIES = libbmsim.la

common_source = \
src/actions.cpp \
src/ageing.cpp \
src/bytecontainer.cpp \
src/calculations.cpp \
src/checksums.cpp \
src/conditionals.cpp \
src/context.cpp \
src/counters.cpp \
src/crc_tables.h \
src/debugger.cpp \
src/deparser.cpp \
src/dev_mgr.cpp \
src/dev_mgr_bmi.cpp \
src/dev_mgr_packet_in.cpp \
src/event_logger.cpp \
src/expressions.cpp \
src/extern.cpp \
src/extract.h \
src/fields.cpp \
src/headers.cpp \
src/learning.cpp \
src/lookup_structures.cpp \
src/logger.cpp \
src/match_units.cpp \
src/match_tables.cpp \
src/md5.c \
src/md5.h \
src/meters.cpp \
src/options_parse.cpp \
src/P4Objects.cpp \
src/packet.cpp \
src/parser.cpp \
src/pcap_file.cpp \
src/pipeline.cpp \
src/port_monitor.cpp \
src/phv.cpp \
src/phv_source.cpp \
src/stateful.cpp \
src/switch.cpp \
src/simple_pre.cpp \
src/simple_pre_lag.cpp \
src/transport.cpp \
src/utils.h \
src/xxhash.c \
src/xxhash.h

common_include = \
include/bm_sim/actions.h \
include/bm_sim/ageing.h \
include/bm_sim/bignum.h \
include/bm_sim/bytecontainer.h \
include/bm_sim/calculations.h \
include/bm_sim/checksums.h \
include/bm_sim/conditionals.h \
include/bm_sim/context.h \
include/bm_sim/control_flow.h \
include/bm_sim/counters.h \
include/bm_sim/data.h \
include/bm_sim/debugger.h \
include/bm_sim/deparser.h \
include/bm_sim/dev_mgr.h \
include/bm_sim/entries.h \
include/bm_sim/event_logger.h \
include/bm_sim/expressions.h \
include/bm_sim/extern.h \
include/bm_sim/fields.h \
include/bm_sim/field_lists.h \
include/bm_sim/handle_mgr.h \
include/bm_sim/headers.h \
include/bm_sim/header_stacks.h \
include/bm_sim/learning.h \
include/bm_sim/logger.h \
include/bm_sim/lpm_trie.h \
include/bm_sim/lookup_structures.h \
include/bm_sim/match_error_codes.h \
include/bm_sim/match_tables.h \
include/bm_sim/match_units.h \
include/bm_sim/match_key_types.h \
include/bm_sim/meters.h \
include/bm_sim/named_p4object.h \
include/bm_sim/nn.h \
include/bm_sim/options_parse.h \
include/bm_sim/P4Objects.h \
include/bm_sim/packet.h \
include/bm_sim/packet_buffer.h \
include/bm_sim/packet_handler.h \
include/bm_sim/parser.h \
include/bm_sim/pcap_file.h \
include/bm_sim/phv.h \
include/bm_sim/phv_forward.h \
include/bm_sim/phv_source.h \
include/bm_sim/pipeline.h \
include/bm_sim/port_monitor.h \
include/bm_sim/pre.h \
include/bm_sim/queue.h \
include/bm_sim/queueing.h \
include/bm_sim/ras.h \
include/bm_sim/runtime_interface.h \
include/bm_sim/stateful.h \
include/bm_sim/switch.h \
include/bm_sim/simple_pre.h \
include/bm_sim/simple_pre_lag.h \
include/bm_sim/tables.h \
include/bm_sim/transport.h

libbmsim_la_SOURCES = $(common_source) $(common_include)

clean-local:
	find $(srcdir) -type f -name '*.gcda' -o -name '*.gcov' -o -name "*.gcno" -delete
