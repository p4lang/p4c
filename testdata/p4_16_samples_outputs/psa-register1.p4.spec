

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

struct execute_registe_5 {
	bit<32> idx
}

struct EMPTY {
	bit<32> psa_ingress_inp_6
	bit<8> psa_ingress_out_7
	bit<32> psa_ingress_out_8
}
metadata instanceof EMPTY

header ethernet instanceof ethernet_t

regarray reg_0 size 0x400 initval 0

action NoAction args none {
	return
}

action execute_registe_9 args instanceof execute_registe_5 {
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		execute_registe_9
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_6
	mov m.psa_ingress_out_7 0x0
	extract h.ethernet
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_out_7 0x0
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


