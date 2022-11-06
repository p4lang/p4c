
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
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_tmp
	bit<32> MainControlT_tmp_0
	bit<32> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<32> MainControlT_tmp_3
	bit<32> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<16> MainControlT_tmp_6
	bit<16> MainControlT_tmp_7
	bit<16> MainControlT_tmp_8
	bit<16> MainControlT_tmp_11
	bit<16> MainControlT_tmp_12
	bit<16> MainControlT_tmp_13
	bit<16> MainControlT_tmp_15
	bit<16> MainControlT_tmp_16
	bit<16> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<16> MainControlT_tmp_19
	bit<16> MainControlT_tmp_20
	bit<16> MainControlT_tmp_23
	bit<16> MainControlT_tmp_24
	bit<16> MainControlT_tmp_25
	bit<16> MainControlT_tmp_27
	bit<16> MainControlT_tmp_28
	bit<16> MainControlT_tmp_29
	bit<16> MainControlT_tmp_30
	bit<16> MainControlT_tmp_31
	bit<16> MainControlT_tmp_32
	bit<16> MainControlT_tmp_33
	bit<16> MainControlT_tmp_34
	bit<32> MainControlT_hsVar
	bit<32> MainControlT_key
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0

action execute_1 args none {
	jmpneq LABEL_FALSE_2 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_1 m.local_metadata_depth
	add m.MainControlT_tmp_1 0x3
	and m.MainControlT_tmp_1 0x3
	jmpneq LABEL_FALSE_3 m.MainControlT_tmp_1 0x0
	jmp LABEL_END_3
	LABEL_FALSE_3 :	mov m.MainControlT_tmp_0 m.local_metadata_depth
	add m.MainControlT_tmp_0 0x3
	and m.MainControlT_tmp_0 0x3
	jmpneq LABEL_FALSE_4 m.MainControlT_tmp_0 0x1
	mov m.MainControlT_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_5 0xF
	mov m.MainControlT_tmp_6 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_6 0x4
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	and m.MainControlT_tmp_7 0xFFF
	mov m.MainControlT_tmp_8 m.MainControlT_tmp_7
	and m.MainControlT_tmp_8 0xFFF
	mov m.MainControlT_tmp_11 m.MainControlT_tmp_8
	shl m.MainControlT_tmp_11 0x4
	mov m.MainControlT_tmp_12 m.MainControlT_tmp_11
	and m.MainControlT_tmp_12 0xFFF0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_5
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_12
	jmp LABEL_END_3
	LABEL_FALSE_4 :	mov m.MainControlT_tmp m.local_metadata_depth
	add m.MainControlT_tmp 0x3
	and m.MainControlT_tmp 0x3
	jmplt LABEL_END_3 m.MainControlT_tmp 0x1
	mov m.MainControlT_tmp_13 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_13 0xF
	mov m.MainControlT_tmp_15 m.MainControlT_hsVar
	shl m.MainControlT_tmp_15 0x4
	mov m.MainControlT_tmp_16 m.MainControlT_tmp_15
	and m.MainControlT_tmp_16 0xFFF0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_13
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_16
	jmp LABEL_END_3
	LABEL_FALSE_2 :	jmpneq LABEL_END_3 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_4 m.local_metadata_depth
	add m.MainControlT_tmp_4 0x3
	and m.MainControlT_tmp_4 0x3
	jmpneq LABEL_FALSE_7 m.MainControlT_tmp_4 0x0
	mov m.MainControlT_tmp_17 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_17 0xF
	mov m.MainControlT_tmp_18 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_18 0x4
	mov m.MainControlT_tmp_19 m.MainControlT_tmp_18
	and m.MainControlT_tmp_19 0xFFF
	mov m.MainControlT_tmp_20 m.MainControlT_tmp_19
	and m.MainControlT_tmp_20 0xFFF
	mov m.MainControlT_tmp_23 m.MainControlT_tmp_20
	shl m.MainControlT_tmp_23 0x4
	mov m.MainControlT_tmp_24 m.MainControlT_tmp_23
	and m.MainControlT_tmp_24 0xFFF0
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_17
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_24
	jmp LABEL_END_3
	LABEL_FALSE_7 :	mov m.MainControlT_tmp_3 m.local_metadata_depth
	add m.MainControlT_tmp_3 0x3
	and m.MainControlT_tmp_3 0x3
	jmpneq LABEL_FALSE_8 m.MainControlT_tmp_3 0x1
	jmp LABEL_END_3
	LABEL_FALSE_8 :	mov m.MainControlT_tmp_2 m.local_metadata_depth
	add m.MainControlT_tmp_2 0x3
	and m.MainControlT_tmp_2 0x3
	jmplt LABEL_END_3 m.MainControlT_tmp_2 0x1
	mov m.MainControlT_tmp_25 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_25 0xF
	mov m.MainControlT_tmp_27 m.MainControlT_hsVar
	shl m.MainControlT_tmp_27 0x4
	mov m.MainControlT_tmp_28 m.MainControlT_tmp_27
	and m.MainControlT_tmp_28 0xFFF0
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_25
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_28
	LABEL_END_3 :	return
}

table stub {
	key {
		m.MainControlT_key exact
	}
	actions {
		execute_1
	}
	default_action execute_1 args none const
	size 0xF4240
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
	mov m.MainControlT_tmp_29 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_29 0x4
	mov m.MainControlT_tmp_30 m.MainControlT_tmp_29
	and m.MainControlT_tmp_30 0xFFF
	mov m.MainControlT_tmp_31 m.MainControlT_tmp_30
	and m.MainControlT_tmp_31 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_31
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_FALSE_0 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_32 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_32 0x4
	mov m.MainControlT_tmp_33 m.MainControlT_tmp_32
	and m.MainControlT_tmp_33 0xFFF
	mov m.MainControlT_tmp_34 m.MainControlT_tmp_33
	and m.MainControlT_tmp_34 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_34
	jmp LABEL_END_0
	LABEL_FALSE_0 :	jmplt LABEL_END_0 m.local_metadata_depth 0x1
	mov m.MainControlT_key m.MainControlT_hsVar
	LABEL_END_0 :	table stub
	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


