AM_CPPFLAGS = -isystem $(srcdir)/../../third_party/jsoncpp/include -I$(srcdir)/include/ -I$(srcdir)/../bf_lpm_trie/include/
AM_CPPFLAGS += $(DEBUG_CPPFLAGS)
# I chose to add -Weffc++ for this module. I feel that most of it is excessively
# pedantic (like constructior initializers for non POD data), but it may be
# useful :/
AM_CXXFLAGS = $(PICKY_CXXFLAGS) -Weffc++
AM_CFLAGS = $(PICKY_CFLAGS)

noinst_LTLIBRARIES = libbmsim.la

common_source = \
src/actions.cpp \
src/checksums.cpp \
src/deparser.cpp \
src/fields.cpp \
src/P4Objects.cpp \
src/parser.cpp \
src/pipeline.cpp \
src/tables.cpp \
src/xxhash.h \
src/conditionals.cpp \
src/event_logger.cpp \
src/headers.cpp \
src/packet.cpp \
src/phv.cpp \
src/switch.cpp \
src/xxhash.c \
src/learning.cpp \
src/pre.cpp \
src/transport.cpp

common_include = \
include/bm_sim/actions.h \
include/bm_sim/bignum.h \
include/bm_sim/bytecontainer.h \
include/bm_sim/checksums.h \
include/bm_sim/conditionals.h \
include/bm_sim/control_flow.h \
include/bm_sim/data.h \
include/bm_sim/deparser.h \
include/bm_sim/entries.h \
include/bm_sim/event_logger.h \
include/bm_sim/fields.h \
include/bm_sim/handle_mgr.h \
include/bm_sim/headers.h \
include/bm_sim/lpm_trie.h \
include/bm_sim/named_p4object.h \
include/bm_sim/nn.h \
include/bm_sim/P4Objects.h \
include/bm_sim/packet_buffer.h \
include/bm_sim/packet.h \
include/bm_sim/parser.h \
include/bm_sim/phv.h \
include/bm_sim/pipeline.h \
include/bm_sim/queue.h \
include/bm_sim/runtime_interface.h \
include/bm_sim/stateful.h \
include/bm_sim/switch.h \
include/bm_sim/tables.h \
include/bm_sim/learning.h \
include/bm_sim/pre.h \
include/bm_sim/transport.h

libbmsim_la_SOURCES = $(common_source) $(common_include)
