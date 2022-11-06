
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
	bit<16> MainControlT_tmp
	bit<16> MainControlT_tmp_0
	bit<16> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<16> MainControlT_tmp_3
	bit<16> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<32> MainControlT_tmp_6
	bit<16> MainControlT_tmp_7
	bit<16> MainControlT_tmp_8
	bit<16> MainControlT_tmp_9
	bit<16> MainControlT_tmp_10
	bit<16> MainControlT_tmp_13
	bit<16> MainControlT_tmp_14
	bit<16> MainControlT_tmp_15
	bit<16> MainControlT_tmp_16
	bit<16> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<16> MainControlT_tmp_21
	bit<16> MainControlT_tmp_22
	bit<16> MainControlT_tmp_23
	bit<16> MainControlT_tmp_24
	bit<16> MainControlT_tmp_25
	bit<16> MainControlT_tmp_26
	bit<16> MainControlT_tmp_27
	bit<16> MainControlT_tmp_28
	bit<32> MainControlT_key
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0

action execute_1 args none {
	jmpneq LABEL_FALSE_1 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_7 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_7 0xF
	mov m.MainControlT_tmp_8 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_8 0x4
	mov m.MainControlT_tmp_9 m.MainControlT_tmp_8
	and m.MainControlT_tmp_9 0xFFF
	mov m.MainControlT_tmp_10 m.MainControlT_tmp_9
	and m.MainControlT_tmp_10 0xFFF
	mov m.MainControlT_tmp_13 m.MainControlT_tmp_10
	shl m.MainControlT_tmp_13 0x4
	mov m.MainControlT_tmp_14 m.MainControlT_tmp_13
	and m.MainControlT_tmp_14 0xFFF0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_7
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_14
	jmp LABEL_END_2
	LABEL_FALSE_1 :	jmpneq LABEL_END_2 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_15 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_15 0xF
	mov m.MainControlT_tmp_16 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_16 0x4
	mov m.MainControlT_tmp_17 m.MainControlT_tmp_16
	and m.MainControlT_tmp_17 0xFFF
	mov m.MainControlT_tmp_18 m.MainControlT_tmp_17
	and m.MainControlT_tmp_18 0xFFF
	mov m.MainControlT_tmp_21 m.MainControlT_tmp_18
	shl m.MainControlT_tmp_21 0x4
	mov m.MainControlT_tmp_22 m.MainControlT_tmp_21
	and m.MainControlT_tmp_22 0xFFF0
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_15
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_22
	LABEL_END_2 :	return
}

action execute_3 args none {
	drop
	return
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


table stub1 {
	key {
		h.ethernet.etherType exact
	}
	actions {
		execute_3
	}
	default_action execute_3 args none const
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
	mov m.MainControlT_tmp h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp 0x4
	mov m.MainControlT_tmp_0 m.MainControlT_tmp
	and m.MainControlT_tmp_0 0xFFF
	mov m.MainControlT_tmp_1 m.MainControlT_tmp_0
	and m.MainControlT_tmp_1 0xFFF
	mov m.MainControlT_tmp_2 m.MainControlT_tmp_1
	jmpeq LABEL_SWITCH m.MainControlT_tmp_2 0x1
	jmpeq LABEL_SWITCH_0 m.MainControlT_tmp_2 0x2
	jmp LABEL_END_0
	LABEL_SWITCH :	mov m.MainControlT_tmp_23 h.vlan_tag_0.pcp_cfi_vid
	shr m.MainControlT_tmp_23 0x4
	mov m.MainControlT_tmp_24 m.MainControlT_tmp_23
	and m.MainControlT_tmp_24 0xFFF
	mov m.MainControlT_tmp_25 m.MainControlT_tmp_24
	and m.MainControlT_tmp_25 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_25
	table stub
	jmp LABEL_END_0
	LABEL_SWITCH_0 :	table stub1
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_END_0 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_3 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_3 0x4
	mov m.MainControlT_tmp_4 m.MainControlT_tmp_3
	and m.MainControlT_tmp_4 0xFFF
	mov m.MainControlT_tmp_5 m.MainControlT_tmp_4
	and m.MainControlT_tmp_5 0xFFF
	mov m.MainControlT_tmp_6 m.MainControlT_tmp_5
	jmpeq LABEL_SWITCH_1 m.MainControlT_tmp_6 0x1
	jmpeq LABEL_SWITCH_2 m.MainControlT_tmp_6 0x2
	jmp LABEL_END_0
	LABEL_SWITCH_1 :	mov m.MainControlT_tmp_26 h.vlan_tag_1.pcp_cfi_vid
	shr m.MainControlT_tmp_26 0x4
	mov m.MainControlT_tmp_27 m.MainControlT_tmp_26
	and m.MainControlT_tmp_27 0xFFF
	mov m.MainControlT_tmp_28 m.MainControlT_tmp_27
	and m.MainControlT_tmp_28 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_28
	table stub
	jmp LABEL_END_0
	LABEL_SWITCH_2 :	table stub1
	LABEL_END_0 :	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


