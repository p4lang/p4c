


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
	bit<16> dataOffset_res_ecn_ctrl
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct cksum_state_t {
	bit<16> state_0
	bit<16> state_1
}

struct dpdk_pseudo_header_t {
	bit<8> pseudo
	bit<8> pseudo_0
	bit<8> pseudo_1
	bit<16> pseudo_2
	bit<16> pseudo_3
	bit<8> pseudo_4
	bit<16> pseudo_5
	bit<8> pseudo_6
	bit<8> pseudo_7
	bit<32> pseudo_8
	bit<32> pseudo_9
	bit<8> pseudo_10
	bit<8> pseudo_11
	bit<8> pseudo_12
	bit<16> pseudo_13
}

struct psa_ingress_output_metadata_t {
	bit<8> class_of_service
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_output_metadata_t {
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
}

struct psa_egress_deparser_input_metadata_t {
	bit<32> egress_port
}

struct set_error_idx_arg_t {
	bit<8> idx
}

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<16> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> table_entry_index
	bit<8> IngressParser_parser_tmp
	bit<8> IngressParser_parser_tmp_0
	bit<8> IngressParser_parser_tmp_2
	bit<8> IngressParser_parser_tmp_3
	bit<8> IngressParser_parser_tmp_4
	bit<8> IngressParser_parser_tmp_5
	bit<8> IngressParser_parser_tmp_6
	bit<16> IngressParser_parser_tmp_7
	bit<16> IngressParser_parser_tmp_8
	bit<16> IngressParser_parser_tmp_9
	bit<16> IngressParser_parser_tmp_10
	bit<16> IngressParser_parser_tmp_11
	bit<16> IngressParser_parser_tmp_12
	bit<8> IngressParser_parser_tmp_13
	bit<32> IngressParser_parser_tmp_14
	bit<32> IngressParser_parser_tmp_15
	bit<8> IngressParser_parser_tmp_16
	bit<16> IngressParser_parser_tmp_17
	bit<16> IngressParser_parser_tmp_18
	bit<32> IngressParser_parser_tmp_19
	bit<32> IngressParser_parser_tmp_20
	bit<8> IngressParser_parser_tmp_21
	bit<8> IngressParser_parser_tmp_22
	bit<32> IngressParser_parser_tmp_23
	bit<32> IngressParser_parser_tmp_24
	bit<8> IngressParser_parser_tmp_25
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t
header cksum_state instanceof cksum_state_t
header dpdk_pseudo_header instanceof dpdk_pseudo_header_t

regarray parser_error_counts_0 size 0x10001 initval 0x0
action set_error_idx args instanceof set_error_idx_arg_t {
	entryid m.table_entry_index 
	regadd parser_error_counts_0 m.table_entry_index 1
	return
}

table parser_error_count_and_convert {
	key {
		m.psa_ingress_input_metadata_parser_error exact
	}
	actions {
		set_error_idx
	}
	default_action set_error_idx args idx 0x0 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	mov m.IngressParser_parser_tmp h.ipv4.version_ihl
	and m.IngressParser_parser_tmp 0xF
	mov m.IngressParser_parser_tmp_0 m.IngressParser_parser_tmp
	and m.IngressParser_parser_tmp_0 0xF
	jmpeq LABEL_TRUE m.IngressParser_parser_tmp_0 0x5
	mov m.IngressParser_parser_tmp_13 0x0
	jmp LABEL_END
	LABEL_TRUE :	mov m.IngressParser_parser_tmp_13 0x1
	LABEL_END :	jmpneq LABEL_END_0 m.IngressParser_parser_tmp_13 0
	mov m.psa_ingress_input_metadata_parser_error 0x7
	jmp INGRESSPARSERIMPL_ACCEPT
	LABEL_END_0 :	mov h.cksum_state.state_0 0x0
	mov m.IngressParser_parser_tmp_2 h.ipv4.version_ihl
	shr m.IngressParser_parser_tmp_2 0x4
	mov m.IngressParser_parser_tmp_3 m.IngressParser_parser_tmp_2
	and m.IngressParser_parser_tmp_3 0xF
	mov m.IngressParser_parser_tmp_4 m.IngressParser_parser_tmp_3
	and m.IngressParser_parser_tmp_4 0xF
	mov m.IngressParser_parser_tmp_14 m.IngressParser_parser_tmp_4
	mov m.IngressParser_parser_tmp_5 h.ipv4.version_ihl
	and m.IngressParser_parser_tmp_5 0xF
	mov m.IngressParser_parser_tmp_6 m.IngressParser_parser_tmp_5
	and m.IngressParser_parser_tmp_6 0xF
	mov m.IngressParser_parser_tmp_15 m.IngressParser_parser_tmp_6
	mov m.IngressParser_parser_tmp_16 h.ipv4.diffserv
	mov m.IngressParser_parser_tmp_17 h.ipv4.totalLen
	mov m.IngressParser_parser_tmp_18 h.ipv4.identification
	mov m.IngressParser_parser_tmp_7 h.ipv4.flags_fragOffset
	shr m.IngressParser_parser_tmp_7 0xD
	mov m.IngressParser_parser_tmp_8 m.IngressParser_parser_tmp_7
	and m.IngressParser_parser_tmp_8 0x7
	mov m.IngressParser_parser_tmp_9 m.IngressParser_parser_tmp_8
	and m.IngressParser_parser_tmp_9 0x7
	mov m.IngressParser_parser_tmp_19 m.IngressParser_parser_tmp_9
	mov m.IngressParser_parser_tmp_10 h.ipv4.flags_fragOffset
	and m.IngressParser_parser_tmp_10 0x1FFF
	mov m.IngressParser_parser_tmp_11 m.IngressParser_parser_tmp_10
	and m.IngressParser_parser_tmp_11 0x1FFF
	mov m.IngressParser_parser_tmp_20 m.IngressParser_parser_tmp_11
	mov m.IngressParser_parser_tmp_21 h.ipv4.ttl
	mov m.IngressParser_parser_tmp_22 h.ipv4.protocol
	mov m.IngressParser_parser_tmp_23 h.ipv4.srcAddr
	mov m.IngressParser_parser_tmp_24 h.ipv4.dstAddr
	mov h.dpdk_pseudo_header.pseudo m.IngressParser_parser_tmp_14
	mov h.dpdk_pseudo_header.pseudo_0 m.IngressParser_parser_tmp_15
	mov h.dpdk_pseudo_header.pseudo_1 m.IngressParser_parser_tmp_16
	mov h.dpdk_pseudo_header.pseudo_2 m.IngressParser_parser_tmp_17
	mov h.dpdk_pseudo_header.pseudo_3 m.IngressParser_parser_tmp_18
	mov h.dpdk_pseudo_header.pseudo_4 m.IngressParser_parser_tmp_19
	mov h.dpdk_pseudo_header.pseudo_5 m.IngressParser_parser_tmp_20
	mov h.dpdk_pseudo_header.pseudo_6 m.IngressParser_parser_tmp_21
	mov h.dpdk_pseudo_header.pseudo_7 m.IngressParser_parser_tmp_22
	mov h.dpdk_pseudo_header.pseudo_8 m.IngressParser_parser_tmp_23
	mov h.dpdk_pseudo_header.pseudo_9 m.IngressParser_parser_tmp_24
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_0
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_1
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_2
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_3
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_4
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_5
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_6
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_7
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_8
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_9
	mov m.IngressParser_parser_tmp_12 h.cksum_state.state_0
	jmpeq LABEL_TRUE_0 m.IngressParser_parser_tmp_12 h.ipv4.hdrChecksum
	mov m.IngressParser_parser_tmp_25 0x0
	jmp LABEL_END_1
	LABEL_TRUE_0 :	mov m.IngressParser_parser_tmp_25 0x1
	LABEL_END_1 :	jmpneq LABEL_END_2 m.IngressParser_parser_tmp_25 0
	mov m.psa_ingress_input_metadata_parser_error 0x8
	jmp INGRESSPARSERIMPL_ACCEPT
	LABEL_END_2 :	jmpeq INGRESSPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_TCP :	extract h.tcp
	INGRESSPARSERIMPL_ACCEPT :	jmpeq LABEL_END_3 m.psa_ingress_input_metadata_parser_error 0x0
	table parser_error_count_and_convert
	mov m.psa_ingress_output_metadata_drop 1
	LABEL_END_3 :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


