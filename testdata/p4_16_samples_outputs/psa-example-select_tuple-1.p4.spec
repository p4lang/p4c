

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
		h.ethernet.srcAddr wildcard
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
	jmpneq INGRESSPARSERIMPL_START_0 h.ethernet.etherType 0x800
	jmpneq INGRESSPARSERIMPL_START_0 h.ethernet.srcAddr 0xf00
	jmp INGRESSPARSERIMPL_PARSE_IPV4
	INGRESSPARSERIMPL_START_0 :	mov m.tmpMask h.ethernet.etherType
	and m.tmpMask 0xf00
	jmpneq INGRESSPARSERIMPL_START_1 m.tmpMask 0x800
	jmpneq INGRESSPARSERIMPL_START_1 h.ethernet.srcAddr 0x3883
	jmp INGRESSPARSERIMPL_PARSE_TCP
	INGRESSPARSERIMPL_START_1 :	jmpneq INGRESSPARSERIMPL_ACCEPT h.ethernet.srcAddr 0x678
	jmp INGRESSPARSERIMPL_PARSE_IPV4
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	INGRESSPARSERIMPL_PARSE_TCP :	extract h.tcp
	INGRESSPARSERIMPL_ACCEPT :	table tbl
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


