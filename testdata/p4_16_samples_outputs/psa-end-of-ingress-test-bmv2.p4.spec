
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
	bit<32> psa_ingress_inp_6
	bit<8> psa_ingress_out_7
	bit<8> psa_ingress_out_8
	bit<32> psa_ingress_out_9
	bit<32> psa_ingress_out_10
	bit<48> Ingress_tmp
	bit<32> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<32> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<16> Ingress_tmp_4
	bit<16> Ingress_tmp_5
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

regarray egress_pkt_seen_11 size 0x100 initval 0

apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	extract h.output_data
	mov m.Ingress_tmp h.ethernet.dstAddr
	shr m.Ingress_tmp 0x28
	mov m.Ingress_tmp_0 m.Ingress_tmp
	jmpneq LABEL_TRUE m.Ingress_tmp_0 0x0
	mov m.psa_ingress_out_7 0x0
	jmp LABEL_END
	LABEL_TRUE :	mov m.psa_ingress_out_7 0x1
	LABEL_END :	jmpneq LABEL_FALSE_0 m.psa_ingress_inp_6 0x5
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.Ingress_tmp_1 h.ethernet.dstAddr
	shr m.Ingress_tmp_1 0x20
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	jmpneq LABEL_TRUE_1 m.Ingress_tmp_2 0x0
	mov m.psa_ingress_out_8 0x0
	jmp LABEL_END_0
	LABEL_TRUE_1 :	mov m.psa_ingress_out_8 0x1
	LABEL_END_0 :	mov m.Ingress_tmp_3 h.ethernet.dstAddr
	shr m.Ingress_tmp_3 0x10
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	mov m.psa_ingress_out_9 m.Ingress_tmp_4
	mov m.Ingress_tmp_5 h.ethernet.dstAddr
	mov m.psa_ingress_out_10 m.Ingress_tmp_5
	mov h.output_data.word0 0x8
	jmpneq LABEL_FALSE_2 m.psa_ingress_inp_6 0x0
	mov h.output_data.word0 0x1
	jmp LABEL_END_2
	LABEL_FALSE_2 :	jmpneq LABEL_FALSE_3 m.psa_ingress_inp_6 0x1
	mov h.output_data.word0 0x2
	jmp LABEL_END_2
	LABEL_FALSE_3 :	jmpneq LABEL_FALSE_4 m.psa_ingress_inp_6 0x2
	mov h.output_data.word0 0x3
	jmp LABEL_END_2
	LABEL_FALSE_4 :	jmpneq LABEL_FALSE_5 m.psa_ingress_inp_6 0x3
	mov h.output_data.word0 0x4
	jmp LABEL_END_2
	LABEL_FALSE_5 :	jmpneq LABEL_FALSE_6 m.psa_ingress_inp_6 0x4
	mov h.output_data.word0 0x5
	jmp LABEL_END_2
	LABEL_FALSE_6 :	jmpneq LABEL_FALSE_7 m.psa_ingress_inp_6 0x5
	mov h.output_data.word0 0x6
	jmp LABEL_END_2
	LABEL_FALSE_7 :	jmpneq LABEL_END_2 m.psa_ingress_inp_6 0x6
	mov h.output_data.word0 0x7
	LABEL_END_2 :	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_out_10
	LABEL_DROP :	drop
}


