
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
	bit<16> Egress_tmp_1
	bit<8> Egress_tmp_2
	bit<48> Ingress_tmp
	bit<1> Ingress_tmp_0
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	extract h.output_data
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xffffffff
	mov m.psa_ingress_output_metadata_egress_port m.Ingress_tmp
	jmpneq LABEL_END h.ethernet.dstAddr 0x0
	mov m.psa_ingress_output_metadata_drop 1
	LABEL_END :	mov m.Ingress_tmp_0 h.ethernet.srcAddr
	mov m.psa_ingress_output_metadata_class_of_service m.Ingress_tmp_0
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.output_data
	extract h.ethernet
	extract h.output_data
	mov h.output_data.word0 m.psa_egress_input_metadata_egress_port
	mov m.Egress_tmp_1 m.psa_egress_input_metadata_instance
	mov h.output_data.word1 m.Egress_tmp_1
	mov h.output_data.word2 0x8
	jmpneq LABEL_FALSE_0 m.psa_egress_input_metadata_packet_path 0x0
	mov h.output_data.word2 0x1
	jmp LABEL_END_0
	LABEL_FALSE_0 :	jmpneq LABEL_FALSE_1 m.psa_egress_input_metadata_packet_path 0x1
	mov h.output_data.word2 0x2
	jmp LABEL_END_0
	LABEL_FALSE_1 :	jmpneq LABEL_FALSE_2 m.psa_egress_input_metadata_packet_path 0x2
	mov h.output_data.word2 0x3
	jmp LABEL_END_0
	LABEL_FALSE_2 :	jmpneq LABEL_FALSE_3 m.psa_egress_input_metadata_packet_path 0x3
	mov h.output_data.word2 0x4
	jmp LABEL_END_0
	LABEL_FALSE_3 :	jmpneq LABEL_FALSE_4 m.psa_egress_input_metadata_packet_path 0x4
	mov h.output_data.word2 0x5
	jmp LABEL_END_0
	LABEL_FALSE_4 :	jmpneq LABEL_FALSE_5 m.psa_egress_input_metadata_packet_path 0x5
	mov h.output_data.word2 0x6
	jmp LABEL_END_0
	LABEL_FALSE_5 :	jmpneq LABEL_END_0 m.psa_egress_input_metadata_packet_path 0x6
	mov h.output_data.word2 0x7
	LABEL_END_0 :	mov m.Egress_tmp_2 m.psa_egress_input_metadata_class_of_service
	mov h.output_data.word3 m.Egress_tmp_2
	emit h.ethernet
	emit h.output_data
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


