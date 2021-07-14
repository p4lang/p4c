

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct user_meta_t {
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
	bit<16> local_metadata_data1
	bit<16> local_metadata_data2
	bit<32> Ingress_tbl_0_group_id
	bit<32> Ingress_tbl_0_member_id
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct tbl_0_set_group_id_arg_t {
	bit<32> group_id
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

action NoAction args none {
	return
}

action a1 args instanceof a1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action tbl_0_set_group_id args instanceof tbl_0_set_group_id_arg_t {
	mov m.Ingress_tbl_0_group_id t.group_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_0_set_group_id
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl_0_member_table {
	key {
		m.Ingress_tbl_0_member_id exact
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


selector tbl_0_group_table {
	group_id m.Ingress_tbl_0_group_id
	selector {
		m.local_metadata_data1
		m.local_metadata_data2
	}
	member_id m.Ingress_tbl_0_member_id
	n_groups_max 1024
	n_members_per_group_max 65536
}

apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	table tbl
	table tbl_0_group_table
	table tbl_0_member_table
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


