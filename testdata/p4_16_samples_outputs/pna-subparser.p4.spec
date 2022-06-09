
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray direction size 0x100 initval 0

apply {
	rx m.pna_main_input_metadata_input_port
	invalidate h.ethernet
	invalidate h.ipv4
	extract h.ethernet
	jmpeq MAINPARSERIMPL_COMMONPARSER_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_START_0
	MAINPARSERIMPL_COMMONPARSER_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_START_0 :	tx m.pna_main_output_metadata_output_port
}


