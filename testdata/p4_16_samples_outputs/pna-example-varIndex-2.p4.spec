
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
	bit<32> MainControlT_tmp_1
	bit<16> MainControlT_tmp_2
	bit<16> MainControlT_tmp_3
	bit<32> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<16> MainControlT_tmp_6
	bit<16> MainControlT_tmp_7
	bit<16> MainControlT_tmp_10
	bit<16> MainControlT_tmp_11
	bit<16> MainControlT_tmp_12
	bit<16> MainControlT_tmp_13
	bit<16> MainControlT_tmp_16
	bit<16> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<16> MainControlT_tmp_19
	bit<16> MainControlT_tmp_20
	bit<32> MainControlT_key
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0
action execute_1 args none {
	jmpneq LABEL_FALSE_1 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_5 0xF000
	mov m.MainControlT_tmp_6 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_6 0xFFF
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	and m.MainControlT_tmp_7 0xFFF
	mov m.MainControlT_tmp_10 m.MainControlT_tmp_7
	and m.MainControlT_tmp_10 0xFFF
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_5
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_10
	jmp LABEL_END_2
	LABEL_FALSE_1 :	jmpneq LABEL_END_2 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_11 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_11 0xF000
	mov m.MainControlT_tmp_12 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_12 0xFFF
	mov m.MainControlT_tmp_13 m.MainControlT_tmp_12
	and m.MainControlT_tmp_13 0xFFF
	mov m.MainControlT_tmp_16 m.MainControlT_tmp_13
	and m.MainControlT_tmp_16 0xFFF
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_11
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_16
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
	and m.MainControlT_tmp 0xFFF
	mov m.MainControlT_tmp_0 m.MainControlT_tmp
	and m.MainControlT_tmp_0 0xFFF
	mov m.MainControlT_tmp_1 m.MainControlT_tmp_0
	jmpeq LABEL_SWITCH m.MainControlT_tmp_1 0x1
	jmpeq LABEL_SWITCH_0 m.MainControlT_tmp_1 0x2
	jmp LABEL_END_0
	LABEL_SWITCH :	mov m.MainControlT_tmp_17 h.vlan_tag_0.pcp_cfi_vid
	and m.MainControlT_tmp_17 0xFFF
	mov m.MainControlT_tmp_18 m.MainControlT_tmp_17
	and m.MainControlT_tmp_18 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_18
	table stub
	jmp LABEL_END_0
	LABEL_SWITCH_0 :	table stub1
	jmp LABEL_END_0
	LABEL_FALSE :	jmpneq LABEL_END_0 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_2 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_2 0xFFF
	mov m.MainControlT_tmp_3 m.MainControlT_tmp_2
	and m.MainControlT_tmp_3 0xFFF
	mov m.MainControlT_tmp_4 m.MainControlT_tmp_3
	jmpeq LABEL_SWITCH_1 m.MainControlT_tmp_4 0x1
	jmpeq LABEL_SWITCH_2 m.MainControlT_tmp_4 0x2
	jmp LABEL_END_0
	LABEL_SWITCH_1 :	mov m.MainControlT_tmp_19 h.vlan_tag_1.pcp_cfi_vid
	and m.MainControlT_tmp_19 0xFFF
	mov m.MainControlT_tmp_20 m.MainControlT_tmp_19
	and m.MainControlT_tmp_20 0xFFF
	mov m.MainControlT_key m.MainControlT_tmp_20
	table stub
	jmp LABEL_END_0
	LABEL_SWITCH_2 :	table stub1
	LABEL_END_0 :	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


