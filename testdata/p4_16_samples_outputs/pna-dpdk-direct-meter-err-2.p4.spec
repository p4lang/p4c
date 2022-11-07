

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
	bit<32> local_metadata_port_out
	bit<32> pna_main_output_metadata_output_port
	bit<32> table_entry_index
	bit<32> MainControlT_out1
	bit<32> MainControlT_color_in
	bit<32> MainControlT_tmp
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

metarray meter0_0 size 0x401

regarray direction size 0x100 initval 0

action send args none {
	entryid m.table_entry_index 
	meter meter0_0 m.table_entry_index 0x400 m.MainControlT_color_in m.MainControlT_out1
	jmpneq LABEL_FALSE m.MainControlT_out1 0x1
	mov m.MainControlT_tmp 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainControlT_tmp 0x0
	LABEL_END :	mov m.local_metadata_port_out m.MainControlT_tmp
	return
}

table ipv4_host {
	key {
		h.ipv4.dstAddr exact
	}
	actions {
		send
	}
	default_action send args none const
	size 0x400
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_color_in 0x0
	table ipv4_host
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


