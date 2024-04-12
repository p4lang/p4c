
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

struct ct_next_hop_0_arg_t {
	bit<32> vport
}

struct next_hop1_arg_t {
	bit<32> vport
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<8> local_metadata_timeout
	bit<32> pna_main_output_metadata_output_port
	bit<32> learnArg
	bit<32> learnArg_0
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray direction size 0x100 initval 0
action next_hop1 args instanceof next_hop1_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action add_on_miss_action args none {
	mov m.learnArg 0x0
	learn next_hop1 m.learnArg m.local_metadata_timeout
	return
}

action ct_next_hop_0 args instanceof ct_next_hop_0_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action ct_add_on_miss_action_0 args none {
	mov m.learnArg_0 0x0
	learn ct_next_hop_0 m.learnArg_0 m.local_metadata_timeout
	return
}

learner ipv4_da {
	key {
		h.ipv4.dstAddr
	}
	actions {
		next_hop1 @tableonly
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

learner ct_ipv4_da {
	key {
		h.ipv4.dstAddr
	}
	actions {
		ct_next_hop_0 @tableonly
		ct_add_on_miss_action_0 @defaultonly
	}
	default_action ct_add_on_miss_action_0 args none 
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
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ipv4
	table ipv4_da
	LABEL_END :	table ct_ipv4_da
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


