
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct vlan_tag_h {
	bit<3> pcp
	bit<1> cfi
	bit<12> vid
	bit<16> ether_type
}

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

header ethernet instanceof ethernet_t
header vlan_tag_0 instanceof vlan_tag_h
header vlan_tag_1 instanceof vlan_tag_h


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

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	jmpeq MYIP_PARSE_VLAN_TAG h.ethernet.etherType 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG :	extract h.vlan_tag_0
	jmpeq MYIP_PARSE_VLAN_TAG1 h.vlan_tag_0.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG1 :	extract h.vlan_tag_1
	jmpeq MYIP_PARSE_VLAN_TAG2 h.vlan_tag_1.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG2 :	jmpeq MYIP_ACCEPT 0 0
	mov metadata 0x3
	MYIP_ACCEPT :	jmpnv LABEL_1FALSE h.ethernet
	jmp LABEL_1END
	LABEL_1FALSE :	table tbl
	LABEL_1END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


