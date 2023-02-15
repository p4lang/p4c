
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

struct send_with_mirror_arg_t {
	bit<32> vport
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlImpl_flowTable_ipv4_srcAddr
	bit<32> MainControlImpl_flowTable_ipv4_dstAddr
	bit<8> MainControlImpl_flowTable_ipv4_protocol
	bit<8> mirrorSlot
	bit<16> mirrorSession
	bit<8> mirrorSlot_0
	bit<16> mirrorSession_0
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action send_with_mirror args instanceof send_with_mirror_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	mov m.mirrorSlot 0x3
	mov m.mirrorSession 0x3A
	mirror m.mirrorSlot m.mirrorSession
	return
}

action drop_with_mirror args none {
	drop
	mov m.mirrorSlot_0 0x3
	mov m.mirrorSession_0 0x3E
	mirror m.mirrorSlot_0 m.mirrorSession_0
	return
}

table flowTable {
	key {
		m.MainControlImpl_flowTable_ipv4_srcAddr exact
		m.MainControlImpl_flowTable_ipv4_dstAddr exact
		m.MainControlImpl_flowTable_ipv4_protocol exact
	}
	actions {
		send_with_mirror
		drop_with_mirror
		NoAction
	}
	default_action NoAction args none const
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlImpl_flowTable_ipv4_srcAddr h.ipv4.srcAddr
	mov m.MainControlImpl_flowTable_ipv4_dstAddr h.ipv4.dstAddr
	mov m.MainControlImpl_flowTable_ipv4_protocol h.ipv4.protocol
	table flowTable
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


