

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
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<16> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_class_of_service
	bit<8> psa_ingress_output_metadata_clone
	bit<16> psa_ingress_output_metadata_clone_session_id
	bit<8> psa_ingress_output_metadata_drop
	bit<8> psa_ingress_output_metadata_resubmit
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> psa_egress_input_metadata_class_of_service
	bit<32> psa_egress_input_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<16> psa_egress_input_metadata_instance
	bit<64> psa_egress_input_metadata_egress_timestamp
	bit<16> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
	bit<48> Ingress_tmp_0
	bit<8> Ingress_tmp_1
	bit<48> Ingress_tmp_2
	bit<8> Ingress_tmp_3
	bit<48> Ingress_tmp_4
	bit<8> Ingress_tmp_5
	bit<48> Ingress_tmp_6
	bit<8> Ingress_tmp_7
	bit<48> Ingress_tmp_8
	bit<8> Ingress_tmp_9
	bit<48> Ingress_tmp
	bit<8> Ingress_idx_0
	bit<16> Ingress_orig_data_0
	bit<16> Ingress_next_data_0
}
metadata instanceof metadata_t

regarray reg_0 size 0x6 initval 0

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	extract h.output_data
	jmpnv LABEL_END h.ethernet
	mov m.Ingress_idx_0 h.ethernet.dstAddr
	mov m.Ingress_tmp_0 h.ethernet.dstAddr
	shr m.Ingress_tmp_0 0x8
	mov m.Ingress_tmp_1 m.Ingress_tmp_0
	mov m.Ingress_tmp_2 h.ethernet.dstAddr
	shr m.Ingress_tmp_2 0x8
	mov m.Ingress_tmp_3 m.Ingress_tmp_2
	jmplt LABEL_END_0 m.Ingress_tmp_1 0x1
	jmpgt LABEL_END_0 m.Ingress_tmp_3 0x3
	regrd m.Ingress_orig_data_0 reg_0 m.Ingress_idx_0
	LABEL_END_0 :	mov m.Ingress_tmp_8 h.ethernet.dstAddr
	shr m.Ingress_tmp_8 0x8
	mov m.Ingress_tmp_9 m.Ingress_tmp_8
	jmpneq LABEL_FALSE_1 m.Ingress_tmp_9 0x1
	mov m.Ingress_tmp h.ethernet.dstAddr
	shr m.Ingress_tmp 0x20
	mov m.Ingress_next_data_0 m.Ingress_tmp
	jmp LABEL_END_1
	LABEL_FALSE_1 :	mov m.Ingress_tmp_6 h.ethernet.dstAddr
	shr m.Ingress_tmp_6 0x8
	mov m.Ingress_tmp_7 m.Ingress_tmp_6
	jmpneq LABEL_FALSE_2 m.Ingress_tmp_7 0x2
	mov m.Ingress_next_data_0 m.Ingress_orig_data_0
	jmp LABEL_END_1
	LABEL_FALSE_2 :	mov m.Ingress_tmp_4 h.ethernet.dstAddr
	shr m.Ingress_tmp_4 0x8
	mov m.Ingress_tmp_5 m.Ingress_tmp_4
	jmpneq LABEL_FALSE_3 m.Ingress_tmp_5 0x3
	mov m.Ingress_next_data_0 m.Ingress_orig_data_0
	add m.Ingress_next_data_0 0x1
	jmp LABEL_END_1
	LABEL_FALSE_3 :	mov m.Ingress_orig_data_0 0xdead
	mov m.Ingress_next_data_0 0xbeef
	LABEL_END_1 :	regwr reg_0 m.Ingress_idx_0 m.Ingress_next_data_0
	mov h.output_data.word0 m.Ingress_orig_data_0
	mov h.output_data.word1 m.Ingress_next_data_0
	LABEL_END :	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port 0x1
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


