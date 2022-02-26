

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

struct metadata_t {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<8> psa_ingress_output_metadata_drop
	bit<8> psa_ingress_output_metadata_resubmit
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> psa_egress_input_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<16> psa_egress_input_metadata_instance
	bit<48> Ingress_tmp
	bit<1> Ingress_tmp_0
	bit<48> Ingress_tmp_1
	bit<1> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<16> Ingress_tmp_4
	bit<16> Ingress_tmp_5
	bit<16> Egress_tmp
	bit<8> Egress_tmp_0
	bit<16> Egress_tmp_1
	bit<8> Egress_tmp_2
	bit<16> Egress_tmp_3
	bit<16> Egress_tmp_4
	bit<8> Egress_idx
	bit<16> Egress_write_data
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

regarray egress_pkt_seen_0 size 0x100 initval 0

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	extract h.output_data
	mov m.Ingress_tmp h.ethernet.dstAddr
	shr m.Ingress_tmp 0x28
	mov m.Ingress_tmp_0 m.Ingress_tmp
	jmpneq LABEL_TRUE m.Ingress_tmp_0 0x0
	mov m.psa_ingress_output_metadata_drop 0x0
	jmp LABEL_END
	LABEL_TRUE :	mov m.psa_ingress_output_metadata_drop 0x1
	LABEL_END :	jmpneq LABEL_FALSE_0 m.psa_ingress_input_metadata_packet_path 0x5
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.Ingress_tmp_1 h.ethernet.dstAddr
	shr m.Ingress_tmp_1 0x20
	mov m.Ingress_tmp_2 m.Ingress_tmp_1
	jmpneq LABEL_TRUE_1 m.Ingress_tmp_2 0x0
	mov m.psa_ingress_output_metadata_resubmit 0x0
	jmp LABEL_END_0
	LABEL_TRUE_1 :	mov m.psa_ingress_output_metadata_resubmit 0x1
	LABEL_END_0 :	mov m.Ingress_tmp_3 h.ethernet.dstAddr
	shr m.Ingress_tmp_3 0x10
	mov m.Ingress_tmp_4 m.Ingress_tmp_3
	mov m.psa_ingress_output_metadata_multicast_group m.Ingress_tmp_4
	mov m.Ingress_tmp_5 h.ethernet.dstAddr
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp_5
	mov h.output_data.word0 0x8
	jmpneq LABEL_FALSE_2 m.psa_ingress_input_metadata_packet_path 0x0
	mov h.output_data.word0 0x1
	jmp LABEL_END_2
	LABEL_FALSE_2 :	jmpneq LABEL_FALSE_3 m.psa_ingress_input_metadata_packet_path 0x1
	mov h.output_data.word0 0x2
	jmp LABEL_END_2
	LABEL_FALSE_3 :	jmpneq LABEL_FALSE_4 m.psa_ingress_input_metadata_packet_path 0x2
	mov h.output_data.word0 0x3
	jmp LABEL_END_2
	LABEL_FALSE_4 :	jmpneq LABEL_FALSE_5 m.psa_ingress_input_metadata_packet_path 0x3
	mov h.output_data.word0 0x4
	jmp LABEL_END_2
	LABEL_FALSE_5 :	jmpneq LABEL_FALSE_6 m.psa_ingress_input_metadata_packet_path 0x4
	mov h.output_data.word0 0x5
	jmp LABEL_END_2
	LABEL_FALSE_6 :	jmpneq LABEL_FALSE_7 m.psa_ingress_input_metadata_packet_path 0x5
	mov h.output_data.word0 0x6
	jmp LABEL_END_2
	LABEL_FALSE_7 :	jmpneq LABEL_END_2 m.psa_ingress_input_metadata_packet_path 0x6
	mov h.output_data.word0 0x7
	LABEL_END_2 :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.output_data
	extract h.ethernet
	extract h.output_data
	mov m.Egress_idx h.ethernet.etherType
	mov m.Egress_tmp_1 h.ethernet.etherType
	shr m.Egress_tmp_1 0x8
	mov m.Egress_tmp_2 m.Egress_tmp_1
	jmpneq LABEL_FALSE_9 m.Egress_tmp_2 0xc0
	regrd m.Egress_tmp_3 egress_pkt_seen_0 m.Egress_idx
	mov h.output_data.word0 m.Egress_tmp_3
	jmp LABEL_END_9
	LABEL_FALSE_9 :	mov m.Egress_tmp h.ethernet.etherType
	shr m.Egress_tmp 0x8
	mov m.Egress_tmp_0 m.Egress_tmp
	jmpneq LABEL_FALSE_10 m.Egress_tmp_0 0xc1
	mov m.Egress_write_data h.ethernet.srcAddr
	regwr egress_pkt_seen_0 m.Egress_idx m.Egress_write_data
	jmp LABEL_END_9
	LABEL_FALSE_10 :	jmplt LABEL_TRUE_11 h.ethernet.etherType 0x100
	jmp LABEL_END_11
	LABEL_TRUE_11 :	regwr egress_pkt_seen_0 m.Egress_idx 0x1
	LABEL_END_11 :	mov h.output_data.word1 m.psa_egress_input_metadata_egress_port
	mov m.Egress_tmp_4 m.psa_egress_input_metadata_instance
	mov h.output_data.word2 m.Egress_tmp_4
	mov h.output_data.word3 0x8
	jmpneq LABEL_FALSE_12 m.psa_egress_input_metadata_packet_path 0x0
	mov h.output_data.word3 0x1
	jmp LABEL_END_9
	LABEL_FALSE_12 :	jmpneq LABEL_FALSE_13 m.psa_egress_input_metadata_packet_path 0x1
	mov h.output_data.word3 0x2
	jmp LABEL_END_9
	LABEL_FALSE_13 :	jmpneq LABEL_FALSE_14 m.psa_egress_input_metadata_packet_path 0x2
	mov h.output_data.word3 0x3
	jmp LABEL_END_9
	LABEL_FALSE_14 :	jmpneq LABEL_FALSE_15 m.psa_egress_input_metadata_packet_path 0x3
	mov h.output_data.word3 0x4
	jmp LABEL_END_9
	LABEL_FALSE_15 :	jmpneq LABEL_FALSE_16 m.psa_egress_input_metadata_packet_path 0x4
	mov h.output_data.word3 0x5
	jmp LABEL_END_9
	LABEL_FALSE_16 :	jmpneq LABEL_FALSE_17 m.psa_egress_input_metadata_packet_path 0x5
	mov h.output_data.word3 0x6
	jmp LABEL_END_9
	LABEL_FALSE_17 :	jmpneq LABEL_END_9 m.psa_egress_input_metadata_packet_path 0x6
	mov h.output_data.word3 0x7
	LABEL_END_9 :	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


