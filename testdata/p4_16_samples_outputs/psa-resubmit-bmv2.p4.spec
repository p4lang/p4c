
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

struct metadata_t {
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<8> psa_ingress_input_metadata_parser_error
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
	bit<8> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t
header output_data instanceof output_data_t

action send_to_port args none {
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	cast  h.ethernet.dstAddr bit_32 m.psa_ingress_output_metadata_egress_port
	return
}

table tbl_send_to_port {
	actions {
		send_to_port
	}
	default_action send_to_port args none 
	size 0
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	extract h.ethernet
	extract h.output_data
	mov m.psa_ingress_output_metadata_drop 0
	jmpeq LABEL_0FALSE m.psa_ingress_input_metadata_packet_path 0x5
	mov h.ethernet.srcAddr 0x100
	mov m.psa_ingress_output_metadata_resubmit 1
	jmp LABEL_0END
	LABEL_0FALSE :	mov h.ethernet.etherType 0xf00d
	table tbl_send_to_port
	mov h.output_data.word0 0x8
	jmpneq LABEL_1FALSE m.psa_ingress_input_metadata_packet_path 0x0
	mov h.output_data.word0 0x1
	jmp LABEL_0END
	LABEL_1FALSE :	jmpneq LABEL_2FALSE m.psa_ingress_input_metadata_packet_path 0x1
	mov h.output_data.word0 0x2
	jmp LABEL_0END
	LABEL_2FALSE :	jmpneq LABEL_3FALSE m.psa_ingress_input_metadata_packet_path 0x2
	mov h.output_data.word0 0x3
	jmp LABEL_0END
	LABEL_3FALSE :	jmpneq LABEL_4FALSE m.psa_ingress_input_metadata_packet_path 0x3
	mov h.output_data.word0 0x4
	jmp LABEL_0END
	LABEL_4FALSE :	jmpneq LABEL_5FALSE m.psa_ingress_input_metadata_packet_path 0x4
	mov h.output_data.word0 0x5
	jmp LABEL_0END
	LABEL_5FALSE :	jmpneq LABEL_6FALSE m.psa_ingress_input_metadata_packet_path 0x5
	mov h.output_data.word0 0x6
	jmp LABEL_0END
	LABEL_6FALSE :	jmpneq LABEL_0END m.psa_ingress_input_metadata_packet_path 0x6
	mov h.output_data.word0 0x7
	LABEL_0END :	jmpnv LABEL_8FALSE h.ethernet
	emit h.ethernet
	LABEL_8FALSE :	jmpnv LABEL_9FALSE h.output_data
	emit h.output_data
	LABEL_9FALSE :	extract h.ethernet
	jmpnv LABEL_10FALSE h.ethernet
	emit h.ethernet
	LABEL_10FALSE :	jmpnv LABEL_11FALSE h.output_data
	emit h.output_data
	LABEL_11FALSE :	tx m.psa_ingress_output_metadata_egress_port
}


