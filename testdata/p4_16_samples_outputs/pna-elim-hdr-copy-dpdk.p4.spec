
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

struct next_hop_arg_t {
	bit<32> vport
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tmpDir
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header ethernet1 instanceof ethernet_t

regarray direction size 0x100 initval 0

action next_hop args instanceof next_hop_arg_t {
	jmpnv LABEL_FALSE_1 h.ethernet
	validate h.ethernet1
	jmp LABEL_END_1
	LABEL_FALSE_1 :	invalidate h.ethernet1
	LABEL_END_1 :	mov h.ethernet1.dstAddr h.ethernet.dstAddr
	mov h.ethernet1.srcAddr h.ethernet.srcAddr
	mov h.ethernet1.etherType h.ethernet.etherType
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action default_route_drop args none {
	drop
	mov h.ethernet.dstAddr h.ethernet1.dstAddr
	mov h.ethernet.srcAddr h.ethernet1.srcAddr
	mov h.ethernet.etherType h.ethernet1.etherType
	return
}

table ipv4_da_lpm {
	key {
		m.MainControlT_tmpDir lpm
	}
	actions {
		next_hop
		default_route_drop
	}
	default_action default_route_drop args none const
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	regrd m.pna_main_input_metadata_direction direction m.pna_main_input_metadata_input_port
	jmpneq LABEL_FALSE 0x0 m.pna_main_input_metadata_direction
	mov m.MainControlT_tmpDir h.ipv4.srcAddr
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainControlT_tmpDir h.ipv4.dstAddr
	LABEL_END :	jmpnv LABEL_END_0 h.ipv4
	table ipv4_da_lpm
	LABEL_END_0 :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


