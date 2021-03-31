
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

action ingress_drop args none {
	mov m.psa_ingress_output_metadata_drop 1
	return
}

action send_to_port args none {
	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	cast  h.ethernet.dstAddr bit_32 m.psa_ingress_output_metadata_egress_port
	return
}

action clone_1 args none {
	mov m.psa_ingress_output_metadata_clone 1
	mov m.psa_ingress_output_metadata_clone_session_id 0x8
	return
}

table tbl_clone {
	actions {
		clone_1
	}
	default_action clone_1 args none 
	size 0
}


table tbl_ingress_drop {
	actions {
		ingress_drop
	}
	default_action ingress_drop args none 
	size 0
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
	table tbl_clone
	jmpneq LABEL_0FALSE h.ethernet.dstAddr 0x9
	table tbl_ingress_drop
	jmp LABEL_0END
	LABEL_0FALSE :	mov h.ethernet.srcAddr 0xcafe
	table tbl_send_to_port
	LABEL_0END :	emit h.ethernet
	extract h.ethernet
	jmpneq LABEL_1END m.psa_egress_input_metadata_packet_path 0x3
	mov h.ethernet.etherType 0xface
	LABEL_1END :	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
}


