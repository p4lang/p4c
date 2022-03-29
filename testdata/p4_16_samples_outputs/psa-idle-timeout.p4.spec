
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<48> srcAddr2
	bit<48> srcAddr3
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

struct a1_1_arg_t {
	bit<48> param
}

struct a1_2_arg_t {
	bit<48> param
}

struct a1_arg_t {
	bit<48> param
}

struct a2_1_arg_t {
	bit<16> param
}

struct a2_2_arg_t {
	bit<16> param
}

struct a2_arg_t {
	bit<16> param
}

struct EMPTY {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
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

action a1_1 args instanceof a1_1_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a1_2 args instanceof a1_2_arg_t {
	mov h.ethernet.dstAddr t.param
	return
}

action a2 args instanceof a2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action a2_1 args instanceof a2_1_arg_t {
	mov h.ethernet.etherType t.param
	return
}

action a2_2 args instanceof a2_2_arg_t {
	mov h.ethernet.etherType t.param
	return
}

table tbl_idle_timeou_8 {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		a1
		a2
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl_no_idle_tim_10 {
	key {
		h.ethernet.srcAddr2 exact
	}
	actions {
		NoAction
		a1_1
		a2_1
	}
	default_action NoAction args none 
	size 0x10000
}


table tbl_no_idle_tim_11 {
	key {
		h.ethernet.srcAddr2 exact
	}
	actions {
		NoAction
		a1_2
		a2_2
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	table tbl_idle_timeout
	table tbl_no_idle_timeout
	table tbl_no_idle_timeout_prop
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	emit h.ethernet
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


