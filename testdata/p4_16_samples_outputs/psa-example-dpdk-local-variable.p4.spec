
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct psa_ingress_out_0 {
	bit<8> class_of_servic_1
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_3 {
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
}

struct psa_egress_depa_4 {
	bit<32> egress_port
}

struct EMPTY {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
}
metadata instanceof EMPTY

header ethernet instanceof ethernet_t

apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	mov h.ethernet.etherType 0xff
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


