

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
	bit<16> local_metadata__data0
	bit<16> local_metadata__data11
	bit<16> local_metadata__tuple_decl_0_f02
	bit<32> pna_main_output_metadata_output_port
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

rss h_0
regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action a1 args none {
	mov m.local_metadata__data11 m.local_metadata__tuple_decl_0_f02
	rss h_0 m.local_metadata__data0  h.ethernet.dstAddr h.ethernet.etherType
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	table tbl
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


