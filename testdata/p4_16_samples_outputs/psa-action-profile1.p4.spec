

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

struct tbl_set_member__5 {
	bit<32> member_id
}

struct EMPTY {
	bit<32> psa_ingress_inp_6
	bit<8> psa_ingress_out_7
	bit<32> psa_ingress_out_8
	bit<32> Ingress_ap_memb_9
}
metadata instanceof EMPTY

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

action tbl_set_member__10 args instanceof tbl_set_member__5 {
	mov m.Ingress_ap_memb_9 t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_member__10
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ap {
	key {
		m.Ingress_ap_memb_9 exact
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_6
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	mov m.Ingress_ap_memb_9 0x0
	table tbl
	table ap
	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	emit h.ethernet
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


