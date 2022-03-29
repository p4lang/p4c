



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

regarray counter0_0_packets size 0x400 initval 0x0

regarray counter0_0_bytes size 0x400 initval 0x0

regarray counter1_0 size 0x400 initval 0x0

regarray counter2_0 size 0x400 initval 0x0

apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	regadd counter0_0_packets 0x3ff 1
	regadd counter0_0_bytes 0x3ff 0x14
	regadd counter1_0 0x200 1
	regadd counter2_0 0x3ff 0x40
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


