





struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<32> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct execute_1_arg_t {
	bit<32> index
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_port_out
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_color_out
	bit<32> MainControlT_color_in
	bit<32> MainControlT_tmp
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray counter0_0_packets size 0x400 initval 0x0

regarray counter0_0_bytes size 0x400 initval 0x0

regarray counter1_0 size 0x400 initval 0x0

regarray counter2_0 size 0x400 initval 0x0

regarray reg_0 size 0x400 initval 0

metarray meter0_0 size 0x400

regarray direction size 0x100 initval 0

action NoAction args none {
	return
}

action execute_1 args instanceof execute_1_arg_t {
	meter meter0_0 t.index h.ipv4.totalLen m.MainControlT_color_in m.MainControlT_color_out
	jmpneq LABEL_FALSE_0 m.MainControlT_color_out 0x1
	mov m.MainControlT_tmp 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.MainControlT_tmp 0x0
	LABEL_END_0 :	mov m.local_metadata_port_out m.MainControlT_tmp
	regwr reg_0 t.index m.local_metadata_port_out
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		execute_1
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
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_color_in 0x0
	jmpneq LABEL_END m.local_metadata_port_out 0x1
	table tbl
	regadd counter0_0_packets 0x3ff 1
	regadd counter0_0_bytes 0x3ff 0x14
	regadd counter1_0 0x200 1
	regadd counter2_0 0x3ff 0x40
	regrd m.local_metadata_port_out reg_0 0x1
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


