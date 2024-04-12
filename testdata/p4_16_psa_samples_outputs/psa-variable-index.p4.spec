
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct vlan_tag_h {
	bit<16> pcp_cfi_vid
	bit<16> ether_type
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

struct EMPTY_M {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<16> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> local_metadata_depth
	bit<16> local_metadata_ret
	bit<16> Ingress_tmp
	bit<16> Ingress_tmp_0
	bit<16> Ingress_tmp_2
	bit<16> Ingress_tmp_3
	bit<16> Ingress_tmp_5
	bit<16> Ingress_tmp_6
	bit<16> Ingress_tmp_7
	bit<16> Ingress_tmp_8
	bit<16> Ingress_hsVar
	bit<32> Ingress_hsVar_0
	bit<32> Ingress_vid
}
metadata instanceof EMPTY_M

header ethernet instanceof ethernet_t
header vlan_tag_0 instanceof vlan_tag_h
header vlan_tag_1 instanceof vlan_tag_h


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
	mov m.psa_ingress_output_metadata_drop 0x1
	mov m.local_metadata_depth 0x1
	extract h.ethernet
	jmpeq MYIP_PARSE_VLAN_TAG h.ethernet.etherType 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG :	extract h.vlan_tag_0
	add m.local_metadata_depth 0x3
	and m.local_metadata_depth 0x3
	jmpeq MYIP_PARSE_VLAN_TAG1 h.vlan_tag_0.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG1 :	extract h.vlan_tag_1
	add m.local_metadata_depth 0x3
	and m.local_metadata_depth 0x3
	jmpeq MYIP_PARSE_VLAN_TAG2 h.vlan_tag_1.ether_type 0x8100
	jmp MYIP_ACCEPT
	MYIP_PARSE_VLAN_TAG2 :	mov m.psa_ingress_input_metadata_parser_error 0x3
	MYIP_ACCEPT :	jmpneq LABEL_FALSE m.local_metadata_depth 0x0
	mov m.Ingress_tmp h.vlan_tag_0.pcp_cfi_vid
	and m.Ingress_tmp 0xFFF
	mov m.Ingress_tmp_0 m.Ingress_tmp
	and m.Ingress_tmp_0 0xFFF
	mov m.local_metadata_ret m.Ingress_tmp_0
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_FALSE_0 m.local_metadata_depth 0x1
	mov m.Ingress_tmp_2 h.vlan_tag_1.pcp_cfi_vid
	and m.Ingress_tmp_2 0xFFF
	mov m.Ingress_tmp_3 m.Ingress_tmp_2
	and m.Ingress_tmp_3 0xFFF
	mov m.local_metadata_ret m.Ingress_tmp_3
	jmp LABEL_END_0
	LABEL_FALSE_0 :	jmplt LABEL_END_0 m.local_metadata_depth 0x1
	mov m.local_metadata_ret m.Ingress_hsVar
	LABEL_END_0 :	jmpnv LABEL_FALSE_2 h.ethernet
	jmp LABEL_END_3
	LABEL_FALSE_2 :	jmpneq LABEL_FALSE_3 m.local_metadata_depth 0x0
	mov m.Ingress_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	and m.Ingress_tmp_5 0xFFF
	mov m.Ingress_tmp_6 m.Ingress_tmp_5
	and m.Ingress_tmp_6 0xFFF
	mov m.Ingress_vid m.Ingress_tmp_6
	jmp LABEL_END_4
	LABEL_FALSE_3 :	jmpneq LABEL_FALSE_4 m.local_metadata_depth 0x1
	mov m.Ingress_tmp_7 h.vlan_tag_1.pcp_cfi_vid
	and m.Ingress_tmp_7 0xFFF
	mov m.Ingress_tmp_8 m.Ingress_tmp_7
	and m.Ingress_tmp_8 0xFFF
	mov m.Ingress_vid m.Ingress_tmp_8
	jmp LABEL_END_4
	LABEL_FALSE_4 :	jmplt LABEL_END_4 m.local_metadata_depth 0x1
	mov m.Ingress_vid m.Ingress_hsVar_0
	LABEL_END_4 :	mov m.local_metadata_ret m.Ingress_vid
	add m.local_metadata_ret 0x5
	table tbl
	LABEL_END_3 :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


