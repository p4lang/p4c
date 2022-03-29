
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
	bit<16> flags_fragOffse_0
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
	bit<16> dataOffset_res__1
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct do_range_checks_2 {
	bit<16> min1
	bit<16> max1
}

struct do_range_checks_3 {
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
	bit<32> pna_main_input__4
	bit<32> local_metadata__5
	bit<32> local_metadata__6
	bit<32> local_metadata__7
	bit<32> pna_main_output_8
	bit<32> MainControlT_tm_9
	bit<32> MainControlT_tm_10
	bit<32> MainControlT_tm_11
	bit<32> MainControlT_tm_12
	bit<32> MainControlT_tm_13
	bit<32> MainControlT_tm_14
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t
header MainControlT_hd_15 instanceof tcp_t

action do_range_checks_16 args instanceof do_range_checks_2 {
	mov h.MainControlT_hd_15 h.tcp
	jmpgt LABEL_FALSE_1 t.min1 h.MainControlT_hd_15.srcPort
	jmpgt LABEL_FALSE_1 h.MainControlT_hd_15.srcPort t.max1
	mov m.MainControlT_tm_13 0x1
	jmp LABEL_END_1
	LABEL_FALSE_1 :	mov m.MainControlT_tm_13 0x0
	LABEL_END_1 :	mov m.local_metadata__5 m.MainControlT_tm_13
	return
}

action next_hop args instanceof next_hop_arg_t {
	mov m.pna_main_output_8 t.vport
	return
}

action add_on_miss_act_17 args none {
	learn next_hop 0x0
	return
}

action do_range_checks_18 args instanceof do_range_checks_3 {
	jmpgt LABEL_FALSE_2 t.min1 h.tcp.srcPort
	jmpgt LABEL_FALSE_2 h.tcp.srcPort t.max1
	jmpgt LABEL_FALSE_3 0x32 h.tcp.srcPort
	jmpgt LABEL_FALSE_3 h.tcp.srcPort 0x64
	mov m.MainControlT_tm_12 m.local_metadata__6
	jmp LABEL_END_3
	LABEL_FALSE_3 :	mov m.MainControlT_tm_12 m.local_metadata__7
	LABEL_END_3 :	mov m.MainControlT_tm_11 m.MainControlT_tm_12
	jmp LABEL_END_2
	LABEL_FALSE_2 :	mov m.MainControlT_tm_11 m.local_metadata__6
	LABEL_END_2 :	mov m.local_metadata__5 m.MainControlT_tm_11
	return
}

action next_hop2 args instanceof next_hop2_arg_t {
	mov m.pna_main_output_8 t.vport
	mov h.ipv4.srcAddr t.newAddr
	return
}

action add_on_miss_act_19 args none {
	mov m.MainControlT_tm_9 0x0
	mov m.MainControlT_tm_10 0x4d2
	learn next_hop m.MainControlT_tm_9
	return
}

learner ipv4_da {
	key {
		h.ipv4.dstAddr
	}
	actions {
		next_hop @tableonly
		add_on_miss_act_17 @defaultonly
	}
	default_action add_on_miss_act_17 args none 
	size 65536
	timeout 120
}

learner ipv4_da2 {
	key {
		h.ipv4.dstAddr
	}
	actions {
		next_hop2 @tableonly
		add_on_miss_act_19 @defaultonly
		do_range_checks_18
		do_range_checks_16
	}
	default_action add_on_miss_act_19 args none 
	size 65536
	timeout 120
}

apply {
	rx m.pna_main_input__4
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	jmpgt LABEL_FALSE 0x64 h.tcp.srcPort
	jmpgt LABEL_FALSE h.tcp.srcPort 0xc8
	mov m.MainControlT_tm_14 0x1
	jmp LABEL_END
	LABEL_FALSE :	mov m.MainControlT_tm_14 0x0
	LABEL_END :	mov m.local_metadata__5 m.MainControlT_tm_14
	jmpnv LABEL_END_0 h.ipv4
	table ipv4_da
	table ipv4_da2
	LABEL_END_0 :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_8
}


