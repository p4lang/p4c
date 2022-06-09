
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

struct udp_t {
	bit<16> src_port
	bit<16> dst_port
	bit<16> length
	bit<16> checksum
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_pass
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header udp instanceof udp_t

regarray direction size 0x100 initval 0

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	extract h.udp
	MAINPARSERIMPL_ACCEPT :	recircid m.pna_main_input_metadata_pass
	jmpeq LABEL_END m.pna_main_input_metadata_pass 0x4
	add h.udp.src_port 0x1
	recirculate
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	emit h.udp
	tx m.pna_main_output_metadata_output_port
}


