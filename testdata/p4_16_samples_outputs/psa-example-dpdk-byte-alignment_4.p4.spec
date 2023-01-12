

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

struct dpdk_pseudo_header_t {
	bit<4> pseudo
	bit<4> pseudo_0
	bit<3> pseudo_1
	bit<13> pseudo_2
	bit<32> pseudo_3
}

struct cksum_state_t {
	bit<16> state_0
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

struct forward_arg_t {
	bit<32> port
	bit<32> srcAddr
}

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata__fwd_metadata_old_srcAddr0
	bit<8> IngressDeparser_deparser_tmp
	bit<8> IngressDeparser_deparser_tmp_0
	bit<8> IngressDeparser_deparser_tmp_1
	bit<8> IngressDeparser_deparser_tmp_2
	bit<8> IngressDeparser_deparser_tmp_3
	bit<16> IngressDeparser_deparser_tmp_4
	bit<16> IngressDeparser_deparser_tmp_5
	bit<16> IngressDeparser_deparser_tmp_6
	bit<16> IngressDeparser_deparser_tmp_7
	bit<16> IngressDeparser_deparser_tmp_8
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t
header dpdk_pseudo_header instanceof dpdk_pseudo_header_t
header cksum_state instanceof cksum_state_t

action NoAction args none {
	return
}

action drop_1 args none {
	mov m.psa_ingress_output_metadata_drop 1
	return
}

action forward args instanceof forward_arg_t {
	mov m.local_metadata__fwd_metadata_old_srcAddr0 h.ipv4.srcAddr
	mov h.ipv4.srcAddr t.srcAddr
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port t.port
	return
}

table route {
	key {
		h.ipv4.dstAddr lpm
	}
	actions {
		forward
		drop_1
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq INGRESSPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_TCP :	extract h.tcp
	INGRESSPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ipv4
	table route
	LABEL_END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	mov h.cksum_state.state_0 0x0
	mov m.IngressDeparser_deparser_tmp h.ipv4.version_ihl
	and m.IngressDeparser_deparser_tmp 0xF
	mov m.IngressDeparser_deparser_tmp_0 m.IngressDeparser_deparser_tmp
	and m.IngressDeparser_deparser_tmp_0 0xF
	mov h.dpdk_pseudo_header.pseudo m.IngressDeparser_deparser_tmp_0
	mov m.IngressDeparser_deparser_tmp_1 h.ipv4.version_ihl
	shr m.IngressDeparser_deparser_tmp_1 0x4
	mov m.IngressDeparser_deparser_tmp_2 m.IngressDeparser_deparser_tmp_1
	and m.IngressDeparser_deparser_tmp_2 0xF
	mov m.IngressDeparser_deparser_tmp_3 m.IngressDeparser_deparser_tmp_2
	and m.IngressDeparser_deparser_tmp_3 0xF
	mov h.dpdk_pseudo_header.pseudo_0 m.IngressDeparser_deparser_tmp_3
	mov m.IngressDeparser_deparser_tmp_4 h.ipv4.flags_fragOffset
	and m.IngressDeparser_deparser_tmp_4 0x7
	mov m.IngressDeparser_deparser_tmp_5 m.IngressDeparser_deparser_tmp_4
	and m.IngressDeparser_deparser_tmp_5 0x7
	mov h.dpdk_pseudo_header.pseudo_1 m.IngressDeparser_deparser_tmp_5
	mov m.IngressDeparser_deparser_tmp_6 h.ipv4.flags_fragOffset
	shr m.IngressDeparser_deparser_tmp_6 0x3
	mov m.IngressDeparser_deparser_tmp_7 m.IngressDeparser_deparser_tmp_6
	and m.IngressDeparser_deparser_tmp_7 0x1FFF
	mov m.IngressDeparser_deparser_tmp_8 m.IngressDeparser_deparser_tmp_7
	and m.IngressDeparser_deparser_tmp_8 0x1FFF
	mov h.dpdk_pseudo_header.pseudo_2 m.IngressDeparser_deparser_tmp_8
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_0
	ckadd h.cksum_state.state_0 h.ipv4.diffserv
	ckadd h.cksum_state.state_0 h.ipv4.totalLen
	ckadd h.cksum_state.state_0 h.ipv4.identification
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_1
	ckadd h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_2
	ckadd h.cksum_state.state_0 h.ipv4.ttl
	ckadd h.cksum_state.state_0 h.ipv4.protocol
	ckadd h.cksum_state.state_0 h.ipv4.srcAddr
	ckadd h.cksum_state.state_0 h.ipv4.dstAddr
	mov h.ipv4.hdrChecksum h.cksum_state.state_0
	mov h.cksum_state.state_0 0x0
	cksub h.cksum_state.state_0 h.tcp.checksum
	mov h.dpdk_pseudo_header.pseudo_3 m.local_metadata__fwd_metadata_old_srcAddr0
	cksub h.cksum_state.state_0 h.dpdk_pseudo_header.pseudo_3
	ckadd h.cksum_state.state_0 h.ipv4.srcAddr
	mov h.tcp.checksum h.cksum_state.state_0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


