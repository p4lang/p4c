
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

struct user_meta_t {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
	bit<16> local_metadata__8
	bit<16> local_metadata__9
	bit<48> Ingress_tbl_eth_10
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

table tbl {
	key {
		m.Ingress_tbl_eth_10 exact
		m.local_metadata__8 selector
		m.local_metadata__9 selector
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
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	mov m.Ingress_tbl_eth_10 h.ethernet.srcAddr
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	emit h.ethernet
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


