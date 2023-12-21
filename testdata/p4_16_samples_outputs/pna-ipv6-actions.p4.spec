
struct Ethernet_h {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct mpls_h {
	bit<24> label_tc_stack
	bit<8> ttl
}

struct IPv4_h {
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

struct IPv6_h {
	bit<32> version_trafficClass_flowLabel
	bit<16> payloadLen
	bit<8> nextHdr
	bit<8> hopLimit
	bit<128> srcAddr
	bit<128> dstAddr
}

struct dpdk_pseudo_header_t {
	bit<32> pseudo
	bit<32> pseudo_0
	bit<64> pseudo_1
}

struct ipv6_modify_dstAddr_arg_t {
	bit<32> dstAddr
}

struct set_flowlabel_arg_t {
	bit<32> label
}

struct set_hop_limit_arg_t {
	bit<8> hopLimit
}

struct set_ipv6_version_arg_t {
	bit<32> version
}

struct set_next_hdr_arg_t {
	bit<8> nextHdr
}

struct set_traffic_class_flow_label_arg_t {
	bit<8> trafficClass
	bit<32> label
}

header ethernet instanceof Ethernet_h
header mpls instanceof mpls_h
header ipv4 instanceof IPv4_h
header ipv6 instanceof IPv6_h
header dpdk_pseudo_header instanceof dpdk_pseudo_header_t

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<32> MainControlT_tmp_4
	bit<32> MainControlT_tmp_5
	bit<32> MainControlT_tmp_6
	bit<32> MainControlT_tmp_8
	bit<32> MainControlT_tmp_9
	bit<32> MainControlT_tmp_11
	bit<32> MainControlT_tmp_12
	bit<128> MainControlT_tmp_13
	bit<32> MainControlT_tmp1
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0
action NoAction args none {
	return
}

action Reject args none {
	drop
	return
}

action ipv6_modify_dstAddr args instanceof ipv6_modify_dstAddr_arg_t {
	mov h.dpdk_pseudo_header.pseudo t.dstAddr
	mov h.ipv6.dstAddr h.dpdk_pseudo_header.pseudo
	return
}

action ipv6_swap_addr args none {
	mov h.ipv6.dstAddr h.ipv6.srcAddr
	mov h.ipv6.srcAddr m.MainControlT_tmp_13
	return
}

action set_flowlabel args instanceof set_flowlabel_arg_t {
	mov m.MainControlT_tmp h.ipv6.version_trafficClass_flowLabel
	and m.MainControlT_tmp 0xFFF00000
	mov m.MainControlT_tmp_1 t.label
	and m.MainControlT_tmp_1 0xFFFFF
	mov h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp
	or h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_1
	return
}

action set_traffic_class_flow_label args instanceof set_traffic_class_flow_label_arg_t {
	mov m.MainControlT_tmp_2 h.ipv6.version_trafficClass_flowLabel
	and m.MainControlT_tmp_2 0xF00FFFFF
	mov m.MainControlT_tmp_4 t.trafficClass
	shl m.MainControlT_tmp_4 0x14
	mov m.MainControlT_tmp_5 m.MainControlT_tmp_4
	and m.MainControlT_tmp_5 0xFF00000
	mov h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_2
	or h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_5
	mov m.MainControlT_tmp_6 h.ipv6.version_trafficClass_flowLabel
	and m.MainControlT_tmp_6 0xFFF00000
	mov m.MainControlT_tmp_8 t.label
	and m.MainControlT_tmp_8 0xFFFFF
	mov h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_6
	or h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_8
	mov h.dpdk_pseudo_header.pseudo_0 m.MainControlT_tmp1
	mov h.ipv6.srcAddr h.dpdk_pseudo_header.pseudo_0
	return
}

action set_ipv6_version args instanceof set_ipv6_version_arg_t {
	mov m.MainControlT_tmp_9 h.ipv6.version_trafficClass_flowLabel
	and m.MainControlT_tmp_9 0xFFFFFFF
	mov m.MainControlT_tmp_11 t.version
	shl m.MainControlT_tmp_11 0x1C
	mov m.MainControlT_tmp_12 m.MainControlT_tmp_11
	and m.MainControlT_tmp_12 0xF0000000
	mov h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_9
	or h.ipv6.version_trafficClass_flowLabel m.MainControlT_tmp_12
	return
}

action set_next_hdr args instanceof set_next_hdr_arg_t {
	mov h.ipv6.nextHdr t.nextHdr
	return
}

action set_hop_limit args instanceof set_hop_limit_arg_t {
	mov h.ipv6.hopLimit t.hopLimit
	return
}

table filter_tbl {
	key {
		h.ipv6.srcAddr exact
	}
	actions {
		ipv6_modify_dstAddr
		ipv6_swap_addr
		set_flowlabel
		set_traffic_class_flow_label
		set_ipv6_version
		set_next_hdr
		set_hop_limit
		Reject
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_IPV4 h.ethernet.etherType 0x800
	jmpeq MAINPARSERIMPL_IPV6 h.ethernet.etherType 0x86DD
	jmpeq MAINPARSERIMPL_MPLS h.ethernet.etherType 0x8847
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_MPLS :	extract h.mpls
	MAINPARSERIMPL_IPV4 :	extract h.ipv4
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_IPV6 :	extract h.ipv6
	MAINPARSERIMPL_ACCEPT :	mov h.dpdk_pseudo_header.pseudo_1 0x76
	mov m.MainControlT_tmp_13 h.dpdk_pseudo_header.pseudo_1
	table filter_tbl
	emit h.ethernet
	emit h.mpls
	emit h.ipv6
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


