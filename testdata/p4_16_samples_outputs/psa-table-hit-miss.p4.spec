
struct EMPTY_M {
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
metadata instanceof EMPTY_M

action NoAction args none {
	return
}

action remove_header args none {
	invalidate h
	return
}

table tbl {
	key {
		h.srcAddr exact
	}
	actions {
		NoAction
		remove_header
	}
	default_action NoAction args none 
	size 0
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	extract h
	table tbl_0
	jmpnh LABEL_0END
	invalidate h
	LABEL_0END :	table tbl_0
	jmph LABEL_1END
	validate h
	LABEL_1END :	table tbl_0
	jmph LABEL_2END
	validate h
	LABEL_2END :	table tbl_0
	jmpnh LABEL_3END
	invalidate h
	LABEL_3END :	tx m.psa_ingress_output_metadata_egress_port
}


