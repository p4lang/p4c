

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
	bit<16> flags_fragOffse_0
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

struct tbl_set_group_i_1 {
	bit<32> group_id
}

struct main_metadata_t {
	bit<32> pna_main_input__2
	bit<16> local_metadata__3
	bit<32> pna_main_output_4
	bit<32> MainControlT_as_5
	bit<32> MainControlT_as_6
}
metadata instanceof main_metadata_t

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

action NoAction args none {
	return
}

action a1 args instanceof a1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action tbl_set_group_i_7 args instanceof tbl_set_group_i_1 {
	mov m.MainControlT_as_5 t.group_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_group_i_7
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table as {
	key {
		m.MainControlT_as_6 exact
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


selector as_sel {
	group_id m.MainControlT_as_group_id
	selector {
		m.local_metadata__3
	}
	member_id m.MainControlT_as_member_id
	n_groups_max 1024
	n_members_per_group_max 65536
}

apply {
	rx m.pna_main_input__2
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_as_6 0x0
	mov m.MainControlT_as_5 0x0
	table tbl
	table as_sel
	table as
	emit h.ethernet
	emit h.ipv4
	tx m.pna_main_output_4
}


