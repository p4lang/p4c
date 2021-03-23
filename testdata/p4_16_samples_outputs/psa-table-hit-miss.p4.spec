
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

action NoAction_0 args none {
	return
}

action remove_header args none {
	invalidate h
	return
}

action ifHit args none {
	invalidate h
	return
}

action ifHit_2 args none {
	invalidate h
	return
}

action ifMiss args none {
	validate h
	return
}

action ifMiss_2 args none {
	validate h
	return
}

table tbl_0 {
	key {
		h.srcAddr exact
	}
	actions {
		NoAction_0
		remove_header
	}
	default_action NoAction_0 args none 
	size 0
}


table tbl_ifHit {
	actions {
		ifHit
	}
	default_action ifHit args none 
	size 0
}


table tbl_ifMiss {
	actions {
		ifMiss
	}
	default_action ifMiss args none 
	size 0
}


table tbl_ifMiss_0 {
	actions {
		ifMiss_2
	}
	default_action ifMiss_2 args none 
	size 0
}


table tbl_ifHit_0 {
	actions {
		ifHit_2
	}
	default_action ifHit_2 args none 
	size 0
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	extract h
	table tbl_0
	jmpnh LABEL_0END
	table tbl_ifHit
	LABEL_0END :	table tbl_0
	jmph LABEL_1END
	table tbl_ifMiss
	LABEL_1END :	table tbl_0
	jmph LABEL_2END
	table tbl_ifMiss_0
	LABEL_2END :	table tbl_0
	jmpnh LABEL_3END
	table tbl_ifHit_0
	LABEL_3END :	tx m.psa_ingress_output_metadata_egress_port
}


