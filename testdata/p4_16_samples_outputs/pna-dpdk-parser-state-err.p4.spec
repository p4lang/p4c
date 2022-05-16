
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

struct tcp_t {
	bit<16> srcPort
	bit<16> dstPort
	bit<32> seqNo
	bit<32> ackNo
	bit<16> dataOffset_res_ecn_flags
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct do_range_checks_1_arg_t {
	bit<16> min1
	bit<16> max1
}

struct do_range_checks_2_arg_t {
	bit<16> min1
	bit<16> max1
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
	bit<32> local_metadata_rng_result1
	bit<16> local_metadata_min1
	bit<16> local_metadata_max1
	bit<8> local_metadata_timeout
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainParserT_parser_tmp
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<32> MainControlT_tmp_3
	bit<32> learnArg
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

regarray direction size 0x100 initval 0

action do_range_checks_1 args instanceof do_range_checks_1_arg_t {
	jmpgt LABEL_FALSE_2 t.min1 h.tcp.srcPort
	jmpgt LABEL_FALSE_2 h.tcp.srcPort t.max1
	mov m.MainControlT_tmp_1 0x1
	jmp LABEL_END_2
	LABEL_FALSE_2 :	mov m.MainControlT_tmp_1 0x0
	LABEL_END_2 :	mov m.local_metadata_rng_result1 m.MainControlT_tmp_1
	return
}

action next_hop args instanceof next_hop_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action add_on_miss_action args none {
	mov m.learnArg 0x0
	learn next_hop m.learnArg m.local_metadata_timeout
	return
}

action do_range_checks_2 args instanceof do_range_checks_2_arg_t {
	jmpgt LABEL_FALSE_3 t.min1 h.tcp.srcPort
	jmpgt LABEL_FALSE_3 h.tcp.srcPort t.max1
	mov m.MainControlT_tmp_2 0x1
	jmp LABEL_END_3
	LABEL_FALSE_3 :	mov m.MainControlT_tmp_2 0x0
	LABEL_END_3 :	mov m.local_metadata_rng_result1 m.MainControlT_tmp_2
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
		h.ipv4.dstAddr
	}
	actions {
		next_hop2 @tableonly
		add_on_miss_action2 @defaultonly
		do_range_checks_2
		do_range_checks_1
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
	extract h.tcp
	jmpgt LABEL_FALSE m.local_metadata_min1 h.tcp.srcPort
	jmpgt LABEL_FALSE h.tcp.srcPort m.local_metadata_max1
	mov m.MainParserT_parser_tmp 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainParserT_parser_tmp 0x0
	LABEL_END :	mov m.local_metadata_rng_result1 m.MainParserT_parser_tmp
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	jmpgt LABEL_FALSE_0 0x64 h.tcp.srcPort
	jmpgt LABEL_FALSE_0 h.tcp.srcPort 0xc8
	mov m.MainControlT_tmp_3 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.MainControlT_tmp_3 0x0
	LABEL_END_0 :	mov m.local_metadata_rng_result1 m.MainControlT_tmp_3
	jmpnv LABEL_END_1 h.ipv4
	table ipv4_da
	table ipv4_da2
	LABEL_END_1 :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


