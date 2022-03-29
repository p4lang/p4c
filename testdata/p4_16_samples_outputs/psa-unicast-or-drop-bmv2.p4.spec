
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct output_data_t {
	bit<32> word0
	bit<32> word1
	bit<32> word2
	bit<32> word3
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

struct metadata_t {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<8> psa_ingress_out_7
	bit<32> psa_ingress_out_8
	bit<32> psa_ingress_out_9
	bit<48> Ingress_tmp
	bit<32> Ingress_tmp_0
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	extract h.output_data
	mov m.psa_ingress_out_7 0
	mov m.psa_ingress_out_8 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xffffffff
	mov m.psa_ingress_out_9 m.Ingress_tmp
	jmpneq LABEL_END h.ethernet.dstAddr 0x0
	mov m.psa_ingress_out_7 1
	LABEL_END :	mov m.Ingress_tmp_0 h.ethernet.srcAddr
	mov m.psa_ingress_out_6 m.Ingress_tmp_0
	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_out_9
	LABEL_DROP :	drop
}


