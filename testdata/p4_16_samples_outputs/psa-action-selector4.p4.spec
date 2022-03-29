

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct psa_ingress_out_0 {
	bit<8> class_of_servic_1
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_outp_3 {
	bit<8> clone
	bit<16> clone_session_i_2
	bit<8> drop
}

struct psa_egress_depa_4 {
	bit<32> egress_port
}

struct a1_arg_t {
	bit<48> param
}

struct a2_arg_t {
	bit<16> param
}

struct tbl_set_group_i_5 {
	bit<32> group_id
}

struct user_meta_t {
	bit<32> psa_ingress_inp_6
	bit<8> psa_ingress_out_7
	bit<32> psa_ingress_out_8
	bit<16> local_metadata__9
	bit<32> Ingress_as_grou_10
	bit<32> Ingress_as_memb_11
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t

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

action tbl_set_group_i_12 args instanceof tbl_set_group_i_5 {
	mov m.Ingress_as_grou_10 t.group_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_group_i_12
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table as {
	key {
		m.Ingress_as_memb_11 exact
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table foo {
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


selector as_sel {
	group_id m.Ingress_as_group_id
	selector {
		m.local_metadata__9
	}
	member_id m.Ingress_as_member_id
	n_groups_max 1024
	n_members_per_group_max 65536
}

apply {
	rx m.psa_ingress_inp_6
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	table tbl
	jmpnh LABEL_END
	table as_sel
	jmpnh LABEL_END
	table as
	jmpnh LABEL_END
	table foo
	LABEL_END :	table tbl
	jmpnh LABEL_END_2
	table as_sel
	jmpnh LABEL_END_2
	table as
	jmpnh LABEL_END_2
	table foo
	LABEL_END_2 :	table tbl
	jmpnh LABEL_END_5
	table as_sel
	jmpnh LABEL_END_5
	table as
	jmpnh LABEL_FALSE_7
	jmp LABEL_END_5
	LABEL_FALSE_7 :	table foo
	LABEL_END_5 :	table tbl
	jmpnh LABEL_END_8
	table as_sel
	jmpnh LABEL_END_8
	table as
	jmpnh LABEL_FALSE_10
	jmp LABEL_END_8
	LABEL_FALSE_10 :	table foo
	LABEL_END_8 :	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	emit h.ethernet
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


