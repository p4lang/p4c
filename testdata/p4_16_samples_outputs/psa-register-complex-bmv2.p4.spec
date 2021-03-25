

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
	bit<48> Ingress_tmp_2
	bit<48> Ingress_tmp_3
	bit<48> Ingress_tmp
	bit<48> Ingress_tmp_0
	bit<48> Ingress_tmp_1
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

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
	register_write regfile_0 0x1 0x3
	register_write regfile_0 0x2 0x4
	mov m.Ingress_tmp m.Ingress_tmp_0
	add m.Ingress_tmp m.Ingress_tmp_1
	mov h.ethernet.dstAddr m.Ingress_tmp
	add h.ethernet.dstAddr 0xfffffffffffb
	mov m.Ingress_tmp_2 m.Ingress_tmp_0
	add m.Ingress_tmp_2 m.Ingress_tmp_1
	mov m.Ingress_tmp_3 m.Ingress_tmp_2
	add m.Ingress_tmp_3 0xfffffffffffb
	jmpneq LABEL_0END m.Ingress_tmp_3 0x2
	table tbl_send_to_port
	LABEL_0END :	jmpnv LABEL_1FALSE h.ethernet
	emit h.ethernet
	LABEL_1FALSE :	extract h.ethernet
	jmpnv LABEL_2FALSE h.ethernet
	emit h.ethernet
	LABEL_2FALSE :	tx m.psa_ingress_output_metadata_egress_port
}


