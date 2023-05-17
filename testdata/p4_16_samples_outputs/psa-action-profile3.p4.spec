

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct psa_ingress_output_metadata_t {
	bit<8> class_of_service
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_output_metadata_t {
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
}

struct psa_egress_deparser_input_metadata_t {
	bit<32> egress_port
}

struct a1_1_arg_t {
	bit<48> param
}

struct a1_arg_t {
	bit<48> param
}

struct a2_1_arg_t {
	bit<16> param
}

struct a2_arg_t {
	bit<16> param
}

struct tbl2_set_member_id_arg_t {
	bit<32> member_id
}

struct tbl_set_member_id_arg_t {
	bit<32> member_id
}

struct EMPTY {
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<8> psa_ingress_output_metadata_drop
	bit<32> psa_ingress_output_metadata_egress_port
	bit<32> Ingress_ap_member_id
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

action tbl_set_member_id args instanceof tbl_set_member_id_arg_t {
	mov m.Ingress_ap_member_id t.member_id
	return
}

action tbl2_set_member_id args instanceof tbl2_set_member_id_arg_t {
	mov m.Ingress_ap_member_id t.member_id
	return
}

table tbl {
	key {
		h.ethernet.srcAddr exact
	}
	actions {
		tbl_set_member_id
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ap {
	key {
		m.Ingress_ap_member_id exact
	}
	actions {
		NoAction
		a1
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
		tbl2_set_member_id
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x1
	extract h.ethernet
	mov m.Ingress_ap_member_id 0x0
	table tbl
	jmpnh LABEL_END
	table ap
	LABEL_END :	mov m.Ingress_ap_member_id 0x0
	table tbl2
	jmpnh LABEL_END_0
	table ap
	LABEL_END_0 :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP :	drop
}


