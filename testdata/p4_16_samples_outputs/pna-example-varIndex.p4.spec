
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
	bit<32> pna_main_input_metadata_input_port
	bit<32> local_metadata_depth
	bit<16> local_metadata_ethType
	bit<32> pna_main_output_metadata_output_port
	bit<16> MainControlT_tmp
	bit<16> MainControlT_tmp_0
	bit<16> MainControlT_tmp_1
	bit<32> MainControlT_tmp_2
	bit<16> MainControlT_tmp_3
	bit<16> MainControlT_tmp_4
	bit<16> MainControlT_tmp_5
	bit<16> MainControlT_tmp_6
	bit<32> MainControlT_tmp_7
	bit<32> MainControlT_tmp_8
	bit<16> MainControlT_tmp_9
	bit<16> MainControlT_tmp_10
	bit<16> MainControlT_tmp_11
	bit<16> MainControlT_tmp_12
	bit<16> MainControlT_tmp_13
	bit<16> MainControlT_tmp_14
	bit<16> MainControlT_tmp_15
	bit<32> MainControlT_tmp_16
	bit<16> MainControlT_tmp_17
	bit<16> MainControlT_tmp_18
	bit<16> MainControlT_tmp_19
	bit<16> MainControlT_tmp_20
	bit<16> MainControlT_tmp_21
	bit<32> MainControlT_key
	bit<32> MainControlT_tmp_22
	bit<32> MainControlT_tmp_23
	bit<32> MainControlT_tmp_24
}
metadata instanceof main_metadata_t

regarray direction size 0x100 initval 0

action execute_1 args none {
	mov m.MainControlT_tmp_22 m.local_metadata_depth
	add m.MainControlT_tmp_22 0x3
	jmpneq LABEL_END_7 m.MainControlT_tmp_22 0x0
	mov m.local_metadata_ethType h.vlan_tag_0.ether_type
	LABEL_END_7 :	jmpneq LABEL_END_8 m.MainControlT_tmp_22 0x1
	mov m.local_metadata_ethType h.vlan_tag_1.ether_type
	LABEL_END_8 :	mov m.MainControlT_tmp_23 m.local_metadata_depth
	add m.MainControlT_tmp_23 0x3
	jmpneq LABEL_END_9 m.MainControlT_tmp_23 0x0
	mov h.vlan_tag_0.ether_type 0x2
	LABEL_END_9 :	jmpneq LABEL_END_10 m.MainControlT_tmp_23 0x1
	mov h.vlan_tag_1.ether_type 0x2
	LABEL_END_10 :	jmpneq LABEL_END_11 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_3 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_11 :	jmpneq LABEL_END_12 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_3 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_12 :	mov m.MainControlT_tmp_4 m.MainControlT_tmp_3
	and m.MainControlT_tmp_4 0xf
	jmpneq LABEL_END_13 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_5 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_13 :	jmpneq LABEL_END_14 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_5 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_14 :	mov m.MainControlT_tmp_6 m.MainControlT_tmp_5
	shr m.MainControlT_tmp_6 0x3
	mov m.MainControlT_tmp_7 m.MainControlT_tmp_6
	mov m.MainControlT_tmp_8 m.MainControlT_tmp_7
	mov m.MainControlT_tmp_9 m.MainControlT_tmp_8
	mov m.MainControlT_tmp_10 m.MainControlT_tmp_9
	shl m.MainControlT_tmp_10 0x4
	mov m.MainControlT_tmp_11 m.MainControlT_tmp_10
	and m.MainControlT_tmp_11 0xfff0
	jmpneq LABEL_END_15 m.local_metadata_depth 0x0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_4
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_11
	LABEL_END_15 :	jmpneq LABEL_END_16 m.local_metadata_depth 0x1
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_4
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_11
	LABEL_END_16 :	jmpneq LABEL_END_17 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_12 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_17 :	jmpneq LABEL_END_18 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_12 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_18 :	mov m.MainControlT_tmp_13 m.MainControlT_tmp_12
	and m.MainControlT_tmp_13 0xf
	mov m.MainControlT_tmp_24 m.local_metadata_depth
	add m.MainControlT_tmp_24 0x3
	jmpneq LABEL_END_19 m.MainControlT_tmp_24 0x0
	mov m.MainControlT_tmp_14 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_19 :	jmpneq LABEL_END_20 m.MainControlT_tmp_24 0x1
	mov m.MainControlT_tmp_14 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_20 :	mov m.MainControlT_tmp_15 m.MainControlT_tmp_14
	shr m.MainControlT_tmp_15 0x4
	mov m.MainControlT_tmp_16 m.MainControlT_tmp_15
	mov m.MainControlT_tmp_17 m.MainControlT_tmp_16
	mov m.MainControlT_tmp_18 m.MainControlT_tmp_17
	shl m.MainControlT_tmp_18 0x4
	mov m.MainControlT_tmp_19 m.MainControlT_tmp_18
	and m.MainControlT_tmp_19 0xfff0
	jmpneq LABEL_END_21 m.local_metadata_depth 0x0
	mov h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_13
	or h.vlan_tag_0.pcp_cfi_vid m.MainControlT_tmp_19
	LABEL_END_21 :	jmpneq LABEL_END_22 m.local_metadata_depth 0x1
	mov h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_13
	or h.vlan_tag_1.pcp_cfi_vid m.MainControlT_tmp_19
	LABEL_END_22 :	return
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
	size 0xf4240
}


table stub1 {
	key {
		h.ethernet.etherType exact
	}
	actions {
		execute_3
	}
	default_action execute_3 args none const
	size 0xf4240
}


apply {
	rx m.pna_main_input_metadata_input_port
	mov m.local_metadata_depth 0x1
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG h.ethernet.etherType 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG :	extract h.vlan_tag_0
	add m.local_metadata_depth 0x3
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG1 h.vlan_tag_0.ether_type 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG1 :	extract h.vlan_tag_1
	add m.local_metadata_depth 0x3
	jmpeq MAINPARSERIMPL_PARSE_VLAN_TAG2 h.vlan_tag_1.ether_type 0x8100
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_VLAN_TAG2 :	mov m.pna_pre_input_metadata_parser_error 0x3
	MAINPARSERIMPL_ACCEPT :	jmpneq LABEL_END_0 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_0 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_0 :	jmpneq LABEL_END_1 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_0 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_1 :	mov m.MainControlT_tmp_1 m.MainControlT_tmp_0
	shr m.MainControlT_tmp_1 0x4
	mov m.MainControlT_tmp_2 m.MainControlT_tmp_1
	jmpeq LABEL_SWITCH m.MainControlT_tmp_2 0x1
	jmpeq LABEL_SWITCH_0 m.MainControlT_tmp_2 0x2
	jmp LABEL_ENDSWITCH
	LABEL_SWITCH :	jmpneq LABEL_END_2 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp_20 h.vlan_tag_0.pcp_cfi_vid
	LABEL_END_2 :	jmpneq LABEL_END_3 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp_20 h.vlan_tag_1.pcp_cfi_vid
	LABEL_END_3 :	mov m.MainControlT_tmp_21 m.MainControlT_tmp_20
	shr m.MainControlT_tmp_21 0x4
	mov m.MainControlT_key m.MainControlT_tmp_21
	table stub
	jmp LABEL_ENDSWITCH
	LABEL_SWITCH_0 :	jmpneq LABEL_END_4 m.local_metadata_depth 0x0
	mov m.MainControlT_tmp h.vlan_tag_0.ether_type
	LABEL_END_4 :	jmpneq LABEL_END_5 m.local_metadata_depth 0x1
	mov m.MainControlT_tmp h.vlan_tag_1.ether_type
	LABEL_END_5 :	jmpneq LABEL_ENDSWITCH m.MainControlT_tmp h.ethernet.etherType
	table stub1
	LABEL_ENDSWITCH :	emit h.ethernet
	emit h.vlan_tag_0
	emit h.vlan_tag_1
	tx m.pna_main_output_metadata_output_port
}


