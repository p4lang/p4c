
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
	bit<16> MainControlT_tmp_19
	bit<16> MainControlT_tmp_20
	bit<16> MainControlT_tmp_21
	bit<16> MainControlT_tmp_22
	bit<16> MainControlT_tmp_25
	bit<16> MainControlT_tmp_26
	bit<16> MainControlT_tmp_27
	bit<16> MainControlT_tmp_28
	bit<16> MainControlT_tmp_31
	bit<16> MainControlT_tmp_32
	bit<16> MainControlT_tmp_33
	bit<16> MainControlT_tmp_34
	bit<16> MainControlT_tmp_35
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
	and m.MainControlT_tmp_4 0xF000
	mov m.MainControlT_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_5 0xC
	mov m.MainControlT_tmp_6 m.MainControlT_tmp_5
	and m.MainControlT_tmp_6 0x1
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	and m.MainControlT_tmp_7 0x1
	mov m.MainControlT_tmp_11 m.MainControlT_tmp_7
	and m.MainControlT_tmp_11 0xFFF
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_4
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_11
	jmp LABEL_END_7
	LABEL_FALSE_6 :	jmpneq LABEL_END_7 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_12 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_12 0xF000
	mov m.MainControlT_tmp_13 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_13 0xC
	mov m.MainControlT_tmp_14 m.MainControlT_tmp_13
	and m.MainControlT_tmp_14 0x1
	mov m.MainControlT_tmp_15 m.MainControlT_tmp_14
	and m.MainControlT_tmp_15 0x1
	mov m.MainControlT_tmp_19 m.MainControlT_tmp_15
	and m.MainControlT_tmp_19 0xFFF
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_12
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_19
	LABEL_END_7 :	jmpneq LABEL_FALSE_8 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_20 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_20 0xF000
	mov m.MainControlT_tmp_21 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_21 0xFFF
	mov m.MainControlT_tmp_22 m.MainControlT_tmp_21
	and m.MainControlT_tmp_22 0xFFF
	mov m.MainControlT_tmp_25 m.MainControlT_tmp_22
	and m.MainControlT_tmp_25 0xFFF
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_20
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_25
	jmp LABEL_END_9
	LABEL_FALSE_8 :	jmpneq LABEL_END_9 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_26 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_26 0xF000
	mov m.MainControlT_tmp_27 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_27 0xFFF
	mov m.MainControlT_tmp_28 m.MainControlT_tmp_27
	and m.MainControlT_tmp_28 0xFFF
	mov m.MainControlT_tmp_31 m.MainControlT_tmp_28
	and m.MainControlT_tmp_31 0xFFF
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_26
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_31
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
	mov m.MainControlT_tmp_32 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_32 0xFFF
	mov m.MainControlT_tmp_33 m.MainControlT_tmp_32
	and m.MainControlT_tmp_33 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_33
	table stub
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_END_0 m.local_metadata_depth 0x1
	jmpneq LABEL_END_0 h.vlan_tag_1.ether_type h.ethernet.etherType
	mov m.MainControlT_tmp_34 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_34 0xFFF
	mov m.MainControlT_tmp_35 m.MainControlT_tmp_34
	and m.MainControlT_tmp_35 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_35
	table stub
	LABEL_END_0 :	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


