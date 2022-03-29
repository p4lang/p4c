

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

struct tbl2_set_member_5 {
	bit<32> member_id
}

struct tbl_set_member__6 {
	bit<32> member_id
}

struct EMPTY {
	bit<32> psa_ingress_inp_7
	bit<8> psa_ingress_out_8
	bit<32> psa_ingress_out_9
	bit<32> Ingress_ap_memb_10
}
metadata instanceof EMPTY

header ethernet instanceof ethernet_t

action NoAction args none {
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action tbl_set_member__11 args instanceof tbl_set_member__6 {
	mov m.Ingress_ap_memb_10 t.member_id
	return
}

action tbl2_set_member_12 args instanceof tbl2_set_member_5 {
	mov m.Ingress_ap_memb_10 t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_member__11
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ap {
	key {
		m.Ingress_ap_memb_10 exact
	}
	actions {
		NoAction
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl2 {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl2_set_member_12
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_7
	mov m.psa_ingress_out_8 0x0
	extract h.ethernet
	mov m.Ingress_ap_memb_10 0x0
	table tbl
	table ap
	mov m.Ingress_ap_memb_10 0x0
	table tbl2
	table ap
	jmpneq LABEL_DROP m.psa_ingress_out_8 0x0
	emit h.ethernet
	tx m.psa_ingress_out_9
	LABEL_DROP :	drop
}


