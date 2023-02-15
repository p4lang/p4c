

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

struct next_hop2_arg_t {
	bit<32> vport
	bit<32> newAddr
}

struct next_hop_arg_t {
	bit<32> vport
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<8> local_metadata_timeout
	bit<32> local_metadata_port_out
	bit<32> pna_main_output_metadata_output_port
	bit<32> table_entry_index
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> MainControlT_color_out
	bit<32> MainControlT_color_in
	bit<32> MainControlT_tmp_1
	bit<32> learnArg
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

metarray meter0_0 size 0x40001
regarray direction size 0x100 initval 0
action next_hop args instanceof next_hop_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action add_on_miss_action args none {
	entryid m.table_entry_index 
	meter meter0_0 m.table_entry_index 0x400 m.MainControlT_color_in m.MainControlT_color_out
	jmpneq LABEL_FALSE_0 m.MainControlT_color_out 0x1
	mov m.MainControlT_tmp_1 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.MainControlT_tmp_1 0x0
	LABEL_END_0 :	mov m.local_metadata_port_out m.MainControlT_tmp_1
	mov m.learnArg 0x0
	learn next_hop m.learnArg m.local_metadata_timeout
	return
}

action next_hop2 args instanceof next_hop2_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	mov h.ipv4.srcAddr t.newAddr
	return
}

action add_on_miss_action2 args none {
	mov m.MainControlT_tmp 0x0
	mov m.MainControlT_tmp_0 0x4D2
	learn next_hop2 m.MainControlT_tmp m.local_metadata_timeout
	return
}

learner ipv4_da {
	key {
		h.ipv4.dstAddr
	}
	actions {
		next_hop @tableonly
		add_on_miss_action @defaultonly
	}
	default_action add_on_miss_action args none 
	size 0x10000
	timeout {
		10
		30
		60
		120
		300
		43200
		120
		120

		}
}

learner ipv4_da2 {
	key {
		h.ipv4.dstAddr
	}
	actions {
		next_hop2 @tableonly
		add_on_miss_action2 @defaultonly
	}
	default_action add_on_miss_action2 args none 
	size 0x10000
	timeout {
		10
		30
		60
		120
		300
		43200
		120
		120

		}
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_color_in 0x0
	jmpnv LABEL_END h.ipv4
	table ipv4_da
	table ipv4_da2
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


