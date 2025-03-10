

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct set_exception_arg_t {
	bit<32> vport
}

struct tbl_set_group_id_arg_t {
	bit<32> group_id
}

struct tbl_set_member_id_arg_t {
	bit<32> member_id
}

struct main_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<16> local_metadata_data
	bit<32> pna_main_output_metadata_output_port
	bit<32> MainControlT_as_group_id
	bit<32> MainControlT_as_member_id
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

regarray direction size 0x100 initval 0
action a1 args instanceof a1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action set_exception args instanceof set_exception_arg_t {
	mov m.pna_main_output_metadata_output_port t.vport
	return
}

action tbl_set_group_id args instanceof tbl_set_group_id_arg_t {
	mov m.MainControlT_as_group_id t.group_id
	return
}

action tbl_set_member_id args instanceof tbl_set_member_id_arg_t {
	mov m.MainControlT_as_member_id t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_group_id
		tbl_set_member_id
		set_exception
	}
	default_action set_exception args vport 0x0 const
	size 0x10000
}


table as {
	key {
		m.MainControlT_as_member_id exact
	}
	actions {
		a1
		a2
		set_exception @defaultonly
	}
	default_action set_exception args vport 0x0 
	size 0x10000
}


selector as_sel {
	group_id m.MainControlT_as_group_id
	selector {
		m.local_metadata_data
	}
	member_id m.MainControlT_as_member_id
	n_groups_max 0x400
	n_members_per_group_max 0x10000
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_as_member_id 0x0
	mov m.MainControlT_as_group_id 0xFFFFFFFF
	table tbl
	jmpnh LABEL_END
	jmpeq LABEL_END_0 m.MainControlT_as_group_id 0xFFFFFFFF
	table as_sel
	LABEL_END_0 :	table as
	LABEL_END :	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_metadata_output_port
}


