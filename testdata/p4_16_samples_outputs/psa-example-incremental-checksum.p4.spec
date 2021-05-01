

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<4> version
	bit<4> ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<3> flags
	bit<13> fragOffset
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
	bit<4> dataOffset
	bit<3> res
	bit<3> ecn
	bit<6> ctrl
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct cksum_state_t {
	bit<16> state_0
}

struct metadata {
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<8> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_class_of_service
	bit<8> psa_ingress_output_metadata_clone
	bit<16> psa_ingress_output_metadata_clone_session_id
	bit<8> psa_ingress_output_metadata_drop
	bit<8> psa_ingress_output_metadata_resubmit
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> psa_egress_input_metadata_class_of_service
	bit<32> psa_egress_input_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<16> psa_egress_input_metadata_instance
	bit<64> psa_egress_input_metadata_egress_timestamp
	bit<8> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
	bit<32> local_metadata__fwd_metadata_old_srcAddr0
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t
header cksum_state instanceof cksum_state_t

struct forward_arg_t {
	bit<32> port
	bit<32> srcAddr
}

action NoAction args none {
	return
}

action drop args none {
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
		drop
		NoAction
	}
	default_action NoAction args none 
	size 0
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq INGRESSPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_TCP :	extract h.tcp
	INGRESSPARSERIMPL_ACCEPT :	jmpnv LABEL_0END h.ipv4
	table route
	LABEL_0END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	ckadd h.cksum_state.state_0 h.ipv4.version
	ckadd h.cksum_state.state_0 h.ipv4.ihl
	ckadd h.cksum_state.state_0 h.ipv4.diffserv
	ckadd h.cksum_state.state_0 h.ipv4.totalLen
	ckadd h.cksum_state.state_0 h.ipv4.identification
	ckadd h.cksum_state.state_0 h.ipv4.flags
	ckadd h.cksum_state.state_0 h.ipv4.fragOffset
	ckadd h.cksum_state.state_0 h.ipv4.ttl
	ckadd h.cksum_state.state_0 h.ipv4.protocol
	ckadd h.cksum_state.state_0 h.ipv4.srcAddr
	ckadd h.cksum_state.state_0 h.ipv4.dstAddr
	mov h.ipv4.hdrChecksum h.cksum_state.state_0
	cksub h.cksum_state.state_0 h.tcp.checksum
	cksub h.cksum_state.state_0 m.local_metadata__fwd_metadata_old_srcAddr0
	ckadd h.cksum_state.state_0 h.ipv4.srcAddr
	mov h.tcp.checksum h.cksum_state.state_0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


