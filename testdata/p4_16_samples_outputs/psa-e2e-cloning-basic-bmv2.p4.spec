
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
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

action send_to_port args none {
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port 0xfffffffa
	return
}

action send_to_port_0 args none {
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


table tbl_send_to_port_0 {
	actions {
		send_to_port_0
	}
	default_action send_to_port_0 args none 
	size 0
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	extract h.ethernet
	jmpneq LABEL_0FALSE h.ethernet.dstAddr 0x8
	jmpeq LABEL_0FALSE m.psa_ingress_input_metadata_packet_path 0x6
	table tbl_send_to_port
	jmp LABEL_0END
	LABEL_0FALSE :	table tbl_send_to_port_0
	LABEL_0END :	jmpnv LABEL_5FALSE h.ethernet
	emit h.ethernet
	LABEL_5FALSE :	extract h.ethernet
	jmpneq LABEL_1FALSE m.psa_egress_input_metadata_packet_path 0x4
	mov h.ethernet.etherType 0xface
	jmp LABEL_1END
	LABEL_1FALSE :	table tbl_clone
	jmpneq LABEL_2END h.ethernet.dstAddr 0x9
	table tbl_egress_drop
	mov m.psa_egress_output_metadata_clone_session_id 0x9
	LABEL_2END :	jmpneq LABEL_3FALSE m.psa_egress_input_metadata_egress_port 0xfffffffa
	mov h.ethernet.srcAddr 0xbeef
	mov m.psa_egress_output_metadata_clone_session_id 0xa
	jmp LABEL_1END
	LABEL_3FALSE :	jmpneq LABEL_4END h.ethernet.dstAddr 0x8
	mov m.psa_egress_output_metadata_clone_session_id 0xb
	LABEL_4END :	mov h.ethernet.srcAddr 0xcafe
	LABEL_1END :	jmpnv LABEL_6FALSE h.ethernet
	emit h.ethernet
	LABEL_6FALSE :	tx m.psa_ingress_output_metadata_egress_port
}


