
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

struct metadata {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
	bit<48> local_metadata__8
	bit<48> local_metadata__9
	bit<48> local_metadata__10
}
metadata instanceof metadata

action NoAction args none {
	return
}

action forward args none {
	mov m.local_metadata__8 h.srcAddr
	or m.local_metadata__8 h.dstAddr
	mov m.local_metadata__9 h.srcAddr
	and m.local_metadata__9 h.dstAddr
	mov m.local_metadata__10 h.srcAddr
	xor m.local_metadata__10 h.dstAddr
	return
}

table tbl {
	key {
		h.srcAddr exact
	}
	actions {
		forward
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h
	table tbl
	mov h.dstAddr m.local_metadata__8
	mov h.srcAddr m.local_metadata__9
	mov h.etherType m.local_metadata__10
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


