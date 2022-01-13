

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_base_t {
	bit<4> version
	bit<4> ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<3> flags
	bit<13> fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct ipv4_option_timestamp_t {
	bit<8> value
	bit<8> len
	varbit<304> data
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

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct tbl2_set_member_id_arg_t {
	bit<32> member_id
}

struct tbl_set_member_id_arg_t {
	bit<32> member_id
}

header ethernet instanceof ethernet_t
header ipv4_base instanceof ipv4_base_t
header ipv4_option_timestamp instanceof ipv4_option_timestamp_t

struct EMPTY {
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
	bit<32> Ingress_ap_member_id
	bit<8> IngressParser_parser_tmp_1
	bit<32> IngressParser_parser_tmp_2
	bit<32> IngressParser_parser_tmp_3
	bit<32> IngressParser_parser_tmp
	bit<16> IngressParser_parser_tmp16_0
	bit<8> IngressParser_parser_tmp_0
	bit<32> IngressParser_parser_tmp_extract_tmp
}
metadata instanceof EMPTY

action NoAction args none {
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action tbl_set_member_id args instanceof tbl_set_member_id_arg_t {
	mov m.Ingress_ap_member_id t.member_id
	return
}

action tbl2_set_member_id args instanceof tbl2_set_member_id_arg_t {
	mov m.Ingress_ap_member_id t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_member_id
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ap {
	key {
		m.Ingress_ap_member_id exact
	}
	actions {
		NoAction
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl2 {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl2_set_member_id
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	jmpeq MYIP_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4 :	extract h.ipv4_base
	jmpeq MYIP_ACCEPT h.ipv4_base.ihl 0x5
	lookahead m.IngressParser_parser_tmp_0
	jmpeq MYIP_PARSE_IPV4_OPTION_TIMESTAMP m.IngressParser_parser_tmp_0 0x44
	jmp MYIP_ACCEPT
	MYIP_PARSE_IPV4_OPTION_TIMESTAMP :	lookahead m.IngressParser_parser_tmp16_0
	mov m.IngressParser_parser_tmp_1 m.IngressParser_parser_tmp16_0
	mov m.IngressParser_parser_tmp_2 m.IngressParser_parser_tmp_1
	mov m.IngressParser_parser_tmp_3 m.IngressParser_parser_tmp_2
	shl m.IngressParser_parser_tmp_3 0x3
	mov m.IngressParser_parser_tmp m.IngressParser_parser_tmp_3
	add m.IngressParser_parser_tmp 0xfffffff0
	mov m.IngressParser_parser_tmp_extract_tmp m.IngressParser_parser_tmp
	shr m.IngressParser_parser_tmp_extract_tmp 0x3
	extract h.ipv4_option_timestamp m.IngressParser_parser_tmp_extract_tmp
	MYIP_ACCEPT :	mov m.psa_ingress_output_metadata_drop 0
	mov m.psa_ingress_output_metadata_multicast_group 0x0
	mov m.psa_ingress_output_metadata_egress_port 0x0
	mov m.Ingress_ap_member_id 0x0
	table tbl
	table ap
	mov m.Ingress_ap_member_id 0x0
	table tbl2
	table ap
	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.ipv4_base
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


