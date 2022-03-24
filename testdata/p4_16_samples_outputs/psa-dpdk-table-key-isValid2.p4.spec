


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
	bit<80> newfield
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

struct metadata {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<16> local_metadata_data
	bit<8> Ingress_tmp
	bit<8> Ingress_tmp_0
	bit<8> Ingress_key
	bit<48> Ingress_tbl_ethernet_dstAddr
	bit<48> Ingress_tbl_ethernet_srcAddr
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

action NoAction args none {
	return
}

action execute_1 args none {
	mov m.local_metadata_data 0x1
	return
}

table tbl {
	key {
		m.Ingress_key exact
		m.Ingress_tbl_ethernet_dstAddr exact
		m.Ingress_tbl_ethernet_srcAddr exact
	}
	actions {
		NoAction
		execute_1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	mov m.tmpMask h.ethernet.etherType
	and m.tmpMask 0xf00
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 m.tmpMask 0x800
	jmpeq INGRESSPARSERIMPL_PARSE_TCP h.ethernet.etherType 0xd00
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	mov m.tmpMask_0 h.ipv4.protocol
	and m.tmpMask_0 0xfc
	jmpeq INGRESSPARSERIMPL_PARSE_TCP m.tmpMask_0 0x4
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_TCP :	extract h.tcp
	INGRESSPARSERIMPL_ACCEPT :	mov m.Ingress_tmp 1
	jmpv LABEL_END h.ethernet
	mov m.Ingress_tmp 0
	LABEL_END :	mov m.Ingress_tmp_0 1
	jmpv LABEL_END_0 h.ipv4
	mov m.Ingress_tmp_0 0
	LABEL_END_0 :	mov m.Ingress_key m.Ingress_tmp
	jmpeq LABEL_TRUE m.Ingress_key 0x1
	jmpeq LABEL_TRUE m.Ingress_tmp_0 0x1
	mov m.Ingress_key 0x0
	jmp LABEL_END_1
	LABEL_TRUE :	mov m.Ingress_key 0x1
	LABEL_END_1 :	mov m.Ingress_tbl_ethernet_dstAddr h.ethernet.dstAddr
	mov m.Ingress_tbl_ethernet_srcAddr h.ethernet.srcAddr
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


