
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

struct user_meta_t {
	bit<32> psa_ingress_inp_5
	bit<8> psa_ingress_out_6
	bit<32> psa_ingress_out_7
	bit<16> local_metadata__8
	bit<16> Ingress_tmp
	bit<16> Ingress_tmp_0
}
metadata instanceof user_meta_t

header ethernet instanceof ethernet_t

action NoAction args none {
	return
}

action execute_1 args none {
	jmpeq LABEL_FALSE_0 m.local_metadata__8 0x0
	mov m.Ingress_tmp 0x0
	jmp LABEL_END_0
	LABEL_FALSE_0 :	mov m.Ingress_tmp 0x1
	LABEL_END_0 :	mov m.local_metadata__8 m.Ingress_tmp
	add m.local_metadata__8 0x1
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		NoAction
		execute_1
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_inp_5
	mov m.psa_ingress_out_6 0x0
	extract h.ethernet
	jmpeq LABEL_FALSE m.local_metadata__8 0x0
	mov m.Ingress_tmp_0 0x2
	jmp LABEL_END
	LABEL_FALSE :	mov m.Ingress_tmp_0 0x5
	LABEL_END :	mov m.local_metadata__8 m.Ingress_tmp_0
	add m.local_metadata__8 0x5
	table tbl
	jmpneq LABEL_DROP m.psa_ingress_out_6 0x0
	tx m.psa_ingress_out_7
	LABEL_DROP :	drop
}


