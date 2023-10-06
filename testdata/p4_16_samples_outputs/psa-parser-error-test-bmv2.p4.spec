
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

struct ipv6_t {
	bit<32> version_trafficClass_flowLabel
	bit<16> payloadLen
	bit<8> nextHdr
	bit<8> hopLimit
	bit<128> srcAddr
	bit<128> dstAddr
}

struct mpls_t {
	bit<24> label_tc_stack
	bit<8> ttl
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
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<16> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<48> Ingress_tmp
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<48> Ingress_tmp_2
	bit<48> Ingress_tmp_4
	bit<48> Ingress_tmp_5
	bit<48> Ingress_tmp_6
	bit<64> Ingress_tmp_8
	bit<64> Ingress_tmp_9
	bit<64> Ingress_tmp_10
	bit<48> Ingress_tmp_13
	bit<16> Ingress_tmp_14
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header ipv6 instanceof ipv6_t
header mpls instanceof mpls_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmpeq INGRESSPARSERIMPL_PARSE_MPLS h.ethernet.etherType 0x8847
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_MPLS :	extract h.mpls
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	INGRESSPARSERIMPL_ACCEPT :	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xFFFFFFFF
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFFFFFFFF
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	and m.Ingress_tmp_1 0xFFFFFFFF
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp_1
	mov m.Ingress_tmp_14 0x8
	jmpneq LABEL_FALSE m.psa_ingress_input_metadata_parser_error 0x0
	mov m.Ingress_tmp_14 0x1
	jmp LABEL_END
	LABEL_FALSE :	jmpneq LABEL_FALSE_0 m.psa_ingress_input_metadata_parser_error 0x1
	mov m.Ingress_tmp_14 0x2
	jmp LABEL_END
	LABEL_FALSE_0 :	jmpneq LABEL_FALSE_1 m.psa_ingress_input_metadata_parser_error 0x2
	mov m.Ingress_tmp_14 0x3
	jmp LABEL_END
	LABEL_FALSE_1 :	jmpneq LABEL_FALSE_2 m.psa_ingress_input_metadata_parser_error 0x3
	mov m.Ingress_tmp_14 0x4
	jmp LABEL_END
	LABEL_FALSE_2 :	jmpneq LABEL_FALSE_3 m.psa_ingress_input_metadata_parser_error 0x4
	mov m.Ingress_tmp_14 0x5
	jmp LABEL_END
	LABEL_FALSE_3 :	jmpneq LABEL_FALSE_4 m.psa_ingress_input_metadata_parser_error 0x5
	mov m.Ingress_tmp_14 0x6
	jmp LABEL_END
	LABEL_FALSE_4 :	jmpneq LABEL_END m.psa_ingress_input_metadata_parser_error 0x6
	mov m.Ingress_tmp_14 0x7
	LABEL_END :	mov m.Ingress_tmp_2 h.ethernet.dstAddr
	and m.Ingress_tmp_2 0xFFFFFFFF
	mov m.Ingress_tmp_4 m.Ingress_tmp_14
	shl m.Ingress_tmp_4 0x20
	mov m.Ingress_tmp_5 m.Ingress_tmp_4
	and m.Ingress_tmp_5 0xFFFF00000000
	mov h.ethernet.dstAddr m.Ingress_tmp_2
	or h.ethernet.dstAddr m.Ingress_tmp_5
	mov m.Ingress_tmp_6 h.ethernet.dstAddr
	and m.Ingress_tmp_6 0xFFFF00000000
	mov m.Ingress_tmp_8 m.psa_ingress_input_metadata_ingress_timestamp
	and m.Ingress_tmp_8 0xFFFFFFFF
	mov m.Ingress_tmp_9 m.Ingress_tmp_8
	and m.Ingress_tmp_9 0xFFFFFFFF
	mov m.Ingress_tmp_10 m.Ingress_tmp_9
	and m.Ingress_tmp_10 0xFFFFFFFF
	mov m.Ingress_tmp_13 m.Ingress_tmp_10
	and m.Ingress_tmp_13 0xFFFFFFFF
	mov h.ethernet.dstAddr m.Ingress_tmp_6
	or h.ethernet.dstAddr m.Ingress_tmp_13
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.mpls
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


