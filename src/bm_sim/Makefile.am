ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS += \
-I$(srcdir)/../BMI \
-I$(srcdir)/../bf_lpm_trie

# I used to have -Weffc++ for this module. I felt that most of it was excessively
# pedantic (like constructior initializers for non POD data), but it was
# useful. However in some versions of g++, it warns for non-virtual destructors
# in base classes even when the destructor is protected, so I removed it :/
AM_CXXFLAGS += $(COVERAGE_FLAGS) # -Weffc++
AM_CFLAGS += $(COVERAGE_FLAGS)

noinst_LTLIBRARIES = libbmsim.la

libbmsim_la_SOURCES = \
actions.cpp \
ageing.cpp \
bytecontainer.cpp \
calculations.cpp \
checksums.cpp \
conditionals.cpp \
context.cpp \
counters.cpp \
crc_tables.h \
debugger.cpp \
deparser.cpp \
dev_mgr.cpp \
dev_mgr_bmi.cpp \
dev_mgr_packet_in.cpp \
event_logger.cpp \
expressions.cpp \
extern.cpp \
extract.h \
fields.cpp \
headers.cpp \
learning.cpp \
lookup_structures.cpp \
logger.cpp \
lpm_trie.h \
match_units.cpp \
match_tables.cpp \
md5.c \
md5.h \
meters.cpp \
options_parse.cpp \
P4Objects.cpp \
packet.cpp \
parser.cpp \
pcap_file.cpp \
pipeline.cpp \
port_monitor.cpp \
phv.cpp \
phv_source.cpp \
stateful.cpp \
switch.cpp \
simple_pre.cpp \
simple_pre_lag.cpp \
target_parser.cpp \
transport.cpp \
utils.h \
version.cpp \
version.h \
xxhash.c \
xxhash.h
