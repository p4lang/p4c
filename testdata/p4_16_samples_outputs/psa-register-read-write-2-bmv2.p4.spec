

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

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

struct metadata_t {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
	bit<32> psa_ingress_out_8
	bit<48> Ingress_tmp
	bit<8> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<8> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<8> Ingress_tmp_4
	bit<48> Ingress_tmp_5
	bit<8> Ingress_tmp_6
	bit<48> Ingress_tmp_7
	bit<8> Ingress_tmp_8
	bit<48> Ingress_tmp_9
	bit<8> Ingress_idx
	bit<16> Ingress_orig_da_9
	bit<16> Ingress_next_da_10
}
metadata instanceof metadata_t

regarray reg_0 size 0x6 initval 0

apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	extract h.output_data
	jmpnv LABEL_END h.ethernet
	mov m.Ingress_idx h.ethernet.dstAddr
	mov m.Ingress_tmp h.ethernet.dstAddr
	shr m.Ingress_tmp 0x8
	mov m.Ingress_tmp_0 m.Ingress_tmp
	mov m.Ingress_tmp_1 h.ethernet.dstAddr
	shr m.Ingress_tmp_1 0x8
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	jmplt LABEL_END_0 m.Ingress_tmp_0 0x1
	jmpgt LABEL_END_0 m.Ingress_tmp_2 0x3
	regrd m.Ingress_orig_da_9 reg_0 m.Ingress_idx
	LABEL_END_0 :	mov m.Ingress_tmp_7 h.ethernet.dstAddr
	shr m.Ingress_tmp_7 0x8
	mov m.Ingress_tmp_8 m.Ingress_tmp_7
	jmpneq LABEL_FALSE_1 m.Ingress_tmp_8 0x1
	mov m.Ingress_tmp_9 h.ethernet.dstAddr
	shr m.Ingress_tmp_9 0x20
	mov m.Ingress_next_da_10 m.Ingress_tmp_9
	jmp LABEL_END_1
	LABEL_FALSE_1 :	mov m.Ingress_tmp_5 h.ethernet.dstAddr
	shr m.Ingress_tmp_5 0x8
	mov m.Ingress_tmp_6 m.Ingress_tmp_5
	jmpneq LABEL_FALSE_2 m.Ingress_tmp_6 0x2
	mov m.Ingress_next_da_10 m.Ingress_orig_da_9
	jmp LABEL_END_1
	LABEL_FALSE_2 :	mov m.Ingress_tmp_3 h.ethernet.dstAddr
	shr m.Ingress_tmp_3 0x8
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	jmpneq LABEL_FALSE_3 m.Ingress_tmp_4 0x3
	mov m.Ingress_next_da_10 m.Ingress_orig_da_9
	add m.Ingress_next_da_10 0x1
	jmp LABEL_END_1
	LABEL_FALSE_3 :	mov m.Ingress_orig_da_9 0xdead
	mov m.Ingress_next_da_10 0xbeef
	LABEL_END_1 :	regwr reg_0 m.Ingress_idx m.Ingress_next_da_10
	mov h.output_data.word0 m.Ingress_orig_da_9
	mov h.output_data.word1 m.Ingress_next_da_10
	LABEL_END :	mov m.psa_ingress_out_6 0
	mov m.psa_ingress_out_7 0x0
	mov m.psa_ingress_out_8 0x1
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


