

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

struct metadata_t {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
	bit<32> psa_ingress_out_8
	bit<48> Ingress_tmp
}
metadata instanceof metadata_t

header ethernet instanceof ethernet_t

regarray counter_0 size 0x400 initval 0x0

action execute_1 args none {
	regadd counter_0 0x100 1
	return
}

table tbl {
	actions {
		execute_1
	}
	default_action execute_1 args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	mov m.psa_ingress_out_6 0
	mov m.psa_ingress_out_7 0x0
	mov m.Ingress_tmp h.ethernet.dstAddr
	and m.Ingress_tmp 0xffffffff
	mov m.psa_ingress_out_8 m.Ingress_tmp
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	emit h.ethernet
	tx m.psa_ingress_out_8
	LABEL_DROP :	drop
}


