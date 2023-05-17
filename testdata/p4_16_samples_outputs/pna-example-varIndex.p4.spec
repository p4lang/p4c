
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct vlan_tag_h {
	bit<16> pcp_cfi_vid
	bit<16> ether_type
}

header ethernet instanceof ethernet_t
header vlan_tag_0 instanceof vlan_tag_h
header vlan_tag_1 instanceof vlan_tag_h


struct main_metadata_t {
	bit<16> pna_pre_input_metadata_parser_error
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_depth
	bit<16> local_metadata_ethType
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<32> MainControlT_tmp_3
	bit<16> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<16> MainControlT_tmp_6
	bit<16> MainControlT_tmp_7
	bit<16> MainControlT_tmp_11
	bit<16> MainControlT_tmp_12
	bit<16> MainControlT_tmp_13
	bit<16> MainControlT_tmp_14
	bit<16> MainControlT_tmp_15
	bit<16> MainControlT_tmp_16
	bit<16> MainControlT_tmp_20
	bit<16> MainControlT_tmp_21
	bit<16> MainControlT_tmp_22
	bit<16> MainControlT_tmp_23
	bit<16> MainControlT_tmp_24
	bit<16> MainControlT_tmp_25
	bit<16> MainControlT_tmp_28
	bit<16> MainControlT_tmp_29
	bit<16> MainControlT_tmp_30
	bit<16> MainControlT_tmp_31
	bit<16> MainControlT_tmp_32
	bit<16> MainControlT_tmp_33
	bit<16> MainControlT_tmp_36
	bit<16> MainControlT_tmp_37
	bit<16> MainControlT_tmp_38
	bit<16> MainControlT_tmp_39
	bit<16> MainControlT_tmp_40
	bit<16> MainControlT_tmp_41
	bit<16> MainControlT_tmp_42
	bit<16> MainControlT_tmp_43
	bit<16> MainControlT_hsVar
	bit<32> MainControlT_key
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0
action execute_1 args none {
	mov m.MainControlT_tmp_1 m.local_metadata_depth
	add m.MainControlT_tmp_1 0x3
	and m.MainControlT_tmp_1 0x3
	jmpneq LABEL_FALSE_1 m.MainControlT_tmp_1 0x0
	mov m.local_metadata_ethType h.vlan_tag_0.ether_type
	jmp LABEL_END_2
	LABEL_FALSE_1 :	mov m.MainControlT_tmp_0 m.local_metadata_depth
	add m.MainControlT_tmp_0 0x3
	and m.MainControlT_tmp_0 0x3
	jmpneq LABEL_FALSE_2 m.MainControlT_tmp_0 0x1
	mov m.local_metadata_ethType h.vlan_tag_1.ether_type
	jmp LABEL_END_2
	LABEL_FALSE_2 :	mov m.MainControlT_tmp m.local_metadata_depth
	add m.MainControlT_tmp 0x3
	and m.MainControlT_tmp 0x3
	jmplt LABEL_END_2 m.MainControlT_tmp 0x1
	mov m.local_metadata_ethType m.MainControlT_hsVar
	LABEL_END_2 :	mov m.MainControlT_tmp_3 m.local_metadata_depth
	add m.MainControlT_tmp_3 0x3
	and m.MainControlT_tmp_3 0x3
	jmpneq LABEL_FALSE_4 m.MainControlT_tmp_3 0x0
	mov h.vlan_tag_0.ether_type 0x2
	jmp LABEL_END_5
	LABEL_FALSE_4 :	mov m.MainControlT_tmp_2 m.local_metadata_depth
	add m.MainControlT_tmp_2 0x3
	and m.MainControlT_tmp_2 0x3
	jmpneq LABEL_END_5 m.MainControlT_tmp_2 0x1
	mov h.vlan_tag_1.ether_type 0x2
	LABEL_END_5 :	jmpneq LABEL_FALSE_6 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_4 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_4 0xF
	mov m.MainControlT_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_5 0x3
	mov m.MainControlT_tmp_6 m.MainControlT_tmp_5
	and m.MainControlT_tmp_6 0x1
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	and m.MainControlT_tmp_7 0x1
	mov m.MainControlT_tmp_11 m.MainControlT_tmp_7
	shl m.MainControlT_tmp_11 0x4
	mov m.MainControlT_tmp_12 m.MainControlT_tmp_11
	and m.MainControlT_tmp_12 0xFFF0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_4
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_12
	jmp LABEL_END_7
	LABEL_FALSE_6 :	jmpneq LABEL_END_7 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_13 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_13 0xF
	mov m.MainControlT_tmp_14 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_14 0x3
	mov m.MainControlT_tmp_15 m.MainControlT_tmp_14
	and m.MainControlT_tmp_15 0x1
	mov m.MainControlT_tmp_16 m.MainControlT_tmp_15
	and m.MainControlT_tmp_16 0x1
	mov m.MainControlT_tmp_20 m.MainControlT_tmp_16
	shl m.MainControlT_tmp_20 0x4
	mov m.MainControlT_tmp_21 m.MainControlT_tmp_20
	and m.MainControlT_tmp_21 0xFFF0
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_13
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_21
	LABEL_END_7 :	jmpneq LABEL_FALSE_8 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_22 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_22 0xF
	mov m.MainControlT_tmp_23 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_23 0x4
	mov m.MainControlT_tmp_24 m.MainControlT_tmp_23
	and m.MainControlT_tmp_24 0xFFF
	mov m.MainControlT_tmp_25 m.MainControlT_tmp_24
	and m.MainControlT_tmp_25 0xFFF
	mov m.MainControlT_tmp_28 m.MainControlT_tmp_25
	shl m.MainControlT_tmp_28 0x4
	mov m.MainControlT_tmp_29 m.MainControlT_tmp_28
	and m.MainControlT_tmp_29 0xFFF0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_22
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_29
	jmp LABEL_END_9
	LABEL_FALSE_8 :	jmpneq LABEL_END_9 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_30 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_30 0xF
	mov m.MainControlT_tmp_31 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_31 0x4
	mov m.MainControlT_tmp_32 m.MainControlT_tmp_31
	and m.MainControlT_tmp_32 0xFFF
	mov m.MainControlT_tmp_33 m.MainControlT_tmp_32
	and m.MainControlT_tmp_33 0xFFF
	mov m.MainControlT_tmp_36 m.MainControlT_tmp_33
	shl m.MainControlT_tmp_36 0x4
	mov m.MainControlT_tmp_37 m.MainControlT_tmp_36
	and m.MainControlT_tmp_37 0xFFF0
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_30
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_37
	LABEL_END_9 :	return
}

table stub {
	key {
		m.MainControlT_key exact
	}
	actions {
		execute_1
	}
	default_action execute_1 args none const
	size 0x2710
}


apply {
	rx m.pna_main_input_metadata_input_port
	mov m.local_metadata_depth 0x1
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG h.ethernet.etherType 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG :	extract h.vlan_tag_0
	add m.local_metadata_depth 0x3
	and m.local_metadata_depth 0x3
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG1 h.vlan_tag_0.ether_type 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG1 :	extract h.vlan_tag_1
	add m.local_metadata_depth 0x3
	and m.local_metadata_depth 0x3
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG2 h.vlan_tag_1.ether_type 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG2 :	mov m.pna_pre_input_metadata_parser_error 0x3
	MAINPARSERIMPL_ACCEPT :	jmpneq LABEL_FALSE m.local_metadata_depth 0x0
	jmpneq LABEL_FALSE h.vlan_tag_0.ether_type h.ethernet.etherType
	mov m.MainControlT_tmp_38 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_38 0x4
	mov m.MainControlT_tmp_39 m.MainControlT_tmp_38
	and m.MainControlT_tmp_39 0xFFF
	mov m.MainControlT_tmp_40 m.MainControlT_tmp_39
	and m.MainControlT_tmp_40 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_40
	table stub
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_END_0 m.local_metadata_depth 0x1
	jmpneq LABEL_END_0 h.vlan_tag_1.ether_type h.ethernet.etherType
	mov m.MainControlT_tmp_41 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_41 0x4
	mov m.MainControlT_tmp_42 m.MainControlT_tmp_41
	and m.MainControlT_tmp_42 0xFFF
	mov m.MainControlT_tmp_43 m.MainControlT_tmp_42
	and m.MainControlT_tmp_43 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_43
	table stub
	LABEL_END_0 :	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


