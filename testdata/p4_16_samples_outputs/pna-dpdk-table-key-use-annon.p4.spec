
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
	bit<32> local_metadata_key
	bit<8> local_metadata_timeout
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> learnArg
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray direction size 0x100 initval 0

action next_hop args instanceof next_hop_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action add_on_miss_action args none {
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
	mov m.MainControlT_tmp_0 0x4d2
	learn next_hop m.MainControlT_tmp m.local_metadata_timeout
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
	size 65536
	timeout {
		60
		120
		180
		}
}

learner ipv4_da2 {
	key {
		m.local_metadata_key
	}
	actions {
		next_hop2 @tableonly
		add_on_miss_action2 @defaultonly
	}
	default_action add_on_miss_action2 args none 
	size 65536
	timeout {
		60
		120
		180
		}
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ipv4
	table ipv4_da
	table ipv4_da2
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


