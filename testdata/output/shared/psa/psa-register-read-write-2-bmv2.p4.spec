

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

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<48> Ingress_tmp
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<48> Ingress_tmp_3
	bit<48> Ingress_tmp_4
	bit<48> Ingress_tmp_5
	bit<48> Ingress_tmp_7
	bit<48> Ingress_tmp_8
	bit<48> Ingress_tmp_9
	bit<48> Ingress_tmp_11
	bit<48> Ingress_tmp_12
	bit<48> Ingress_tmp_13
	bit<48> Ingress_tmp_15
	bit<48> Ingress_tmp_16
	bit<48> Ingress_tmp_17
	bit<48> Ingress_tmp_19
	bit<48> Ingress_tmp_20
	bit<48> Ingress_tmp_21
	bit<48> Ingress_tmp_22
	bit<48> Ingress_tmp_23
	bit<8> Ingress_idx
	bit<16> Ingress_orig_data
	bit<16> Ingress_next_data
}
metadata instanceof metadata_t

regarray reg_0 size 0x6 initval 0
apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	extract h.output_data
	jmpnv LABEL_END h.ethernet
	mov m.Ingress_tmp_19 h.ethernet.dstAddr
	and m.Ingress_tmp_19 0xFF
	mov m.Ingress_tmp_20 m.Ingress_tmp_19
	and m.Ingress_tmp_20 0xFF
	mov m.Ingress_idx m.Ingress_tmp_20
	mov m.Ingress_tmp h.ethernet.dstAddr
	shr m.Ingress_tmp 0x8
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFF
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	and m.Ingress_tmp_1 0xFF
	mov m.Ingress_tmp_3 h.ethernet.dstAddr
	shr m.Ingress_tmp_3 0x8
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	and m.Ingress_tmp_4 0xFF
	mov m.Ingress_tmp_5 m.Ingress_tmp_4
	and m.Ingress_tmp_5 0xFF
	jmplt LABEL_END_0 m.Ingress_tmp_1 0x1
	jmpgt LABEL_END_0 m.Ingress_tmp_5 0x3
	regrd m.Ingress_orig_data reg_0 m.Ingress_idx
	LABEL_END_0 :	mov m.Ingress_tmp_15 h.ethernet.dstAddr
	shr m.Ingress_tmp_15 0x8
	mov m.Ingress_tmp_16 m.Ingress_tmp_15
	and m.Ingress_tmp_16 0xFF
	mov m.Ingress_tmp_17 m.Ingress_tmp_16
	and m.Ingress_tmp_17 0xFF
	jmpneq LABEL_FALSE_1 m.Ingress_tmp_17 0x1
	mov m.Ingress_tmp_21 h.ethernet.dstAddr
	shr m.Ingress_tmp_21 0x20
	mov m.Ingress_tmp_22 m.Ingress_tmp_21
	and m.Ingress_tmp_22 0xFFFF
	mov m.Ingress_tmp_23 m.Ingress_tmp_22
	and m.Ingress_tmp_23 0xFFFF
	mov m.Ingress_next_data m.Ingress_tmp_23
	jmp LABEL_END_1
	LABEL_FALSE_1 :	mov m.Ingress_tmp_11 h.ethernet.dstAddr
	shr m.Ingress_tmp_11 0x8
	mov m.Ingress_tmp_12 m.Ingress_tmp_11
	and m.Ingress_tmp_12 0xFF
	mov m.Ingress_tmp_13 m.Ingress_tmp_12
	and m.Ingress_tmp_13 0xFF
	jmpneq LABEL_FALSE_2 m.Ingress_tmp_13 0x2
	mov m.Ingress_next_data m.Ingress_orig_data
	jmp LABEL_END_1
	LABEL_FALSE_2 :	mov m.Ingress_tmp_7 h.ethernet.dstAddr
	shr m.Ingress_tmp_7 0x8
	mov m.Ingress_tmp_8 m.Ingress_tmp_7
	and m.Ingress_tmp_8 0xFF
	mov m.Ingress_tmp_9 m.Ingress_tmp_8
	and m.Ingress_tmp_9 0xFF
	jmpneq LABEL_FALSE_3 m.Ingress_tmp_9 0x3
	mov m.Ingress_next_data m.Ingress_orig_data
	add m.Ingress_next_data 0x1
	jmp LABEL_END_1
	LABEL_FALSE_3 :	mov m.Ingress_orig_data 0xDEAD
	mov m.Ingress_next_data 0xBEEF
	LABEL_END_1 :	regwr reg_0 m.Ingress_idx m.Ingress_next_data
	mov h.output_data.word0 m.Ingress_orig_data
	mov h.output_data.word1 m.Ingress_next_data
	LABEL_END :	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port 0x1
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


