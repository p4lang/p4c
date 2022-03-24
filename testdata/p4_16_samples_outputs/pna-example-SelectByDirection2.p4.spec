
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

struct forward_arg_t {
	bit<32> addr
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_meta
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_addr
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

action forward args instanceof forward_arg_t {
	mov m.local_metadata_meta t.addr
	return
}

action default_route_drop args none {
	drop
	return
}

table ipv4_da_lpm {
	key {
		h.ipv4.dstAddr lpm
	}
	actions {
		forward
		default_route_drop
	}
	default_action default_route_drop args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ipv4
	jmpeq LABEL_TRUE_0 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_addr h.ipv4.dstAddr
	jmp LABEL_END_0
	LABEL_TRUE_0 :	mov m.MainControlT_addr h.ipv4.srcAddr
	LABEL_END_0 :	mov m.local_metadata_meta m.MainControlT_addr
	jmpneq LABEL_END m.local_metadata_meta h.ipv4.dstAddr
	table ipv4_da_lpm
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


