
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
	bit<80> newfield
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

struct psa_ingress_out_2 {
	bit<8> class_of_servic_3
	bit<8> clone
	bit<16> clone_session_i_4
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_5 {
	bit<8> clone
	bit<16> clone_session_i_4
	bit<8> drop
}

struct psa_egress_depa_6 {
	bit<32> egress_port
}

struct metadata {
	bit<32> psa_ingress_inp_7
	bit<8> psa_ingress_out_8
	bit<32> psa_ingress_out_9
}
metadata instanceof metadata

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

apply {
	rx m.psa_ingress_inp_7
	mov m.psa_ingress_out_8 0x0
	jmpneq LABEL_DROP m.psa_ingress_out_8 0x0
	emit h.ethernet
	emit h.ipv4
	emit h.tcp
	tx m.psa_ingress_out_9
	LABEL_DROP :	drop
}


