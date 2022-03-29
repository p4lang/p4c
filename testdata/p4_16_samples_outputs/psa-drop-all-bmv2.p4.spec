
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

struct psa_ingress_out_1 {
	bit<8> class_of_servic_2
	bit<8> clone
	bit<16> clone_session_i_3
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_4 {
	bit<8> clone
	bit<16> clone_session_i_3
	bit<8> drop
}

struct psa_egress_depa_5 {
	bit<32> egress_port
}

struct metadata_t {
	bit<32> psa_ingress_inp_6
	bit<8> psa_ingress_out_7
	bit<32> psa_ingress_out_8
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

apply {
	rx m.psa_ingress_inp_6
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	jmpeq INGRESSPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp INGRESSPARSERIMPL_ACCEPT
	INGRESSPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	INGRESSPARSERIMPL_ACCEPT :	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	emit h.ethernet
	emit h.ipv4
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


